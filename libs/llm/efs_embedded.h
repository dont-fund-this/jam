// efs_embedded.h
// Provides EmbeddedFile and embedded_data for vision/llm loose coupling

#ifndef EFS_EMBEDDED_H
#define EFS_EMBEDDED_H

struct EmbeddedFile {
    const char* path;
    const unsigned char* data;
    unsigned int size;
};

extern "C" const EmbeddedFile embedded_data[];

#endif // EFS_EMBEDDED_H
