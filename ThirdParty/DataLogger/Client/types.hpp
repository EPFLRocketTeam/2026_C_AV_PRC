
#define X_prim(T) X(T) X(volatile T)

#define X_PRIMITIVE_TYPES \
    X_prim(bool) \
    X_prim(unsigned char) \
    X_prim(char) \
    X_prim(signed char) \
    X_prim(unsigned short) \
    X_prim(short) \
    X_prim(unsigned int) \
    X_prim(int) \
    X_prim(unsigned long) \
    X_prim(long) \
    X_prim(long long) \
    X_prim(unsigned long long) \
    X_prim(float) \
    X_prim(double)
