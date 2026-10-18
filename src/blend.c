#include "blend.h"
#include "policy.h"
#include <stdio.h>
#include <string.h>

static void failure(char *out, size_t cap, const char *code) {
    snprintf(out, cap, "{\"ok\":false,\"error\":\"%s\",\"release\":%d}", code, BLEND_RELEASE);
}

static int number(const char *text, size_t length, unsigned long maximum, unsigned long *result) {
    unsigned long n = 0;
    if (!length || length > 7) return 0;
    for (size_t i = 0; i < length; ++i) {
        if (text[i] < '0' || text[i] > '9') return 0;
        n = n * 10 + (unsigned long)(text[i] - '0');
        if (n > maximum) return 0;
    }
    *result = n;
    return 1;
}

static void recipe(char *path, char *out, size_t cap) {
    char *slash = strchr(path, '/');
    unsigned long grams = 0, weights[8], allocated[8], remainder[8], total = 0, assigned = 0;
    unsigned count = 0;
    if (!slash || !number(path, (size_t)(slash - path), BLEND_MAX_GRAMS, &grams) || !grams) {
        failure(out, cap, "invalid_mass"); return;
    }
    const char *part = slash + 1;
    for (;;) {
        const char *comma = strchr(part, ',');
        size_t length = comma ? (size_t)(comma - part) : strlen(part);
        if (count == 8 || !number(part, length, 10000UL, &weights[count])) {
            failure(out, cap, "invalid_weights"); return;
        }
        total += weights[count++];
        if (!comma) break;
        part = comma + 1;
    }
    if (count < 2 || total != 10000UL) { failure(out, cap, "weights_must_sum_to_10000"); return; }
    for (unsigned i = 0; i < count; ++i) {
        unsigned long long product = (unsigned long long)grams * weights[i];
        allocated[i] = (unsigned long)(product / 10000ULL);
        remainder[i] = (unsigned long)(product % 10000ULL);
        assigned += allocated[i];
    }
    /* Largest remainder; earlier component wins exact ties. Each can get at most one extra gram. */
    unsigned awarded[8] = {0};
    while (assigned < grams) {
        unsigned best = 0;
        while (awarded[best]) ++best;
        for (unsigned i = best + 1; i < count; ++i)
            if (!awarded[i] && remainder[i] > remainder[best]) best = i;
        ++allocated[best]; awarded[best] = 1; ++assigned;
    }
    int n = snprintf(out, cap, "{\"ok\":true,\"service\":\"blend-lab\",\"release\":%d,\"grams\":%lu,\"parts\":%u,\"weights\":[", BLEND_RELEASE, grams, count);
    size_t used = n > 0 ? (size_t)n : cap;
    for (unsigned i = 0; i < count && used < cap; ++i)
        used += (size_t)snprintf(out + used, cap - used, "%s%lu", i ? "," : "", weights[i]);
    if (used < cap) used += (size_t)snprintf(out + used, cap - used, "],\"allocatedGrams\":[");
    for (unsigned i = 0; i < count && used < cap; ++i)
        used += (size_t)snprintf(out + used, cap - used, "%s%lu", i ? "," : "", allocated[i]);
    if (used < cap) snprintf(out + used, cap - used, "],\"sumGrams\":%lu,\"rounding\":\"largest-remainder-stable\"}", assigned);
}

void blend_answer(const char *line, char *out, size_t cap) {
    char method[8] = "GET", path[1024] = "/health", protocol[16] = "HTTP/1.1";
    if (line && *line && sscanf(line, "%7s %1023s %15s", method, path, protocol) != 3) {
        failure(out, cap, "invalid_request"); return;
    }
    char *query = strchr(path, '?');
    if (query) *query = '\0';
    fprintf(stderr, "blend_lab release=%d method=%s route=%s\n", BLEND_RELEASE, method, path);
    if (strcmp(method, "GET")) { failure(out, cap, "method_not_allowed"); return; }
    if (!strcmp(path, "/") || !strcmp(path, "/health")) {
        snprintf(out, cap, "{\"ok\":true,\"service\":\"blend-lab\",\"release\":%d,\"maxGrams\":%lu,\"maxParts\":8}", BLEND_RELEASE, BLEND_MAX_GRAMS); return;
    }
    if (!strcmp(path, "/recipe")) {
        snprintf(out, cap, "{\"ok\":true,\"example\":\"/blend/1001/5000,3000,2000\",\"units\":\"grams and basis points\",\"release\":%d}", BLEND_RELEASE); return;
    }
    if (!strncmp(path, "/blend/", 7)) { recipe(path + 7, out, cap); return; }
    failure(out, cap, "not_found");
}
