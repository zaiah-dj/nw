#ifndef DEBUG_H
#define DEBUG_H
//A define to step through calls
#if 0
 #define STEP(...) do { \
	fprintf( stderr, __VA_ARGS__ ); getchar(); } while (0)
#else
 #define STEP(...) do { \
	fprintf( stderr, "%20s [ %s: %d ]", __func__, __FILE__, __LINE__ ); \
	getchar(); } while (0)
#endif

 #define SHOWDATA(...) do { \
	fprintf(stderr, "%-30s [ %s %d ] -> ", __func__, __FILE__, __LINE__); \
	fprintf( stderr, __VA_ARGS__ ); } while (0)

#endif
