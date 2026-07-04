#pragma once
// Minimal stb_image stub for build when the real third_party header is absent.
// Replace with the actual upstream header if image loading is required.

#ifdef __cplusplus
extern "C" {
#endif

inline unsigned char* stbi_load(const char* filename, int* x, int* y, int* channels, int desired_channels) {
    if (x) *x = 0; if (y) *y = 0; if (channels) *channels = 0;
    return nullptr;
}
inline void stbi_image_free(void* retval_from_stbi_load) {}

#ifdef __cplusplus
}
#endif
