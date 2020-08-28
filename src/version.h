#ifndef TINK_VERSION_H
#define TINK_VERSION_H

// for cmake
#define TINK_VER_MAJOR 0
#define TINK_VER_MINOR 9
#define TINK_VER_PATCH 0

#define TINK_VERSION  (TINK_VER_MAJOR * 10000 + TINK_VER_MINOR * 100 + TINK_VER_PATCH);
// for source code
#define _VERSION_STR(s) #s
#define _PROJECT_VERSION(major, minor, patch) "v" _VERSION_STR(major.minor.patch)
#define TINK_VERSION_STR _PROJECT_VERSION(TINK_VER_MAJOR, TINK_VER_MINOR, TINK_VER_PATCH)
#endif //TINK_VERSION_H
