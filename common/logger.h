#ifndef LOGGER_H_
#define LOGGER_H_

typedef struct {
  const char *file;
  int line;
  int col;
} SrcLocation;

#define PRINT_ERROR(loc, fmt, ...) \
    fprintf(stderr, "%s:%d:%d: [ERROR]: " fmt "\n", (loc).file, (loc).line, (loc).col, ##__VA_ARGS__)
#define PRINT_BUG(loc, fmt, ...) \
    fprintf(stderr, "%s:%d:%d: [BUG]: " fmt "\n", (loc).file, (loc).line, (loc).col, ##__VA_ARGS__)
#define PRINT_INFO(loc, fmt, ...) \
    fprintf(stderr, "%s:%d:%d: [INFO]: " fmt "\n", (loc).file, (loc).line, (loc).col, ##__VA_ARGS__)
#define PRINT_WARNING(loc, fmt, ...) \
    fprintf(stderr, "%s:%d:%d: [WARNING]: " fmt "\n", (loc).file, (loc).line, (loc).col, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) \
    fprintf(stderr, "%s:%d: [ERROR]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_BUG(fmt, ...) \
    fprintf(stderr, "%s:%d: [BUG]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
    fprintf(stderr, "%s:%d: [INFO]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) \
    fprintf(stderr, "%s:%d: [WARNING]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)


#endif
