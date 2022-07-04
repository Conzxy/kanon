#ifndef KANON_NONCOPYABLE_H
#define KANON_NONCOPYABLE_H

namespace kanon {

/**
 * If a handler class(i.e. resource manager) is trivially copy 
 * to other. Must inherit this class to disable copy ctor and 
 * assignment operator
 *
 * Also, this class don't disable move operation since if not 
 * ensure the base class can be move, the derived class is not
 * allowed to move also. 
 */
struct noncopyable{
#if __cplusplus >= 201103L
    noncopyable() = default;
    noncopyable(noncopyable const&) = delete;
    noncopyable& operator=(noncopyable const&) = delete;
    noncopyable(noncopyable&&) = default;
    noncopyable& operator=(noncopyable&&) = default;

#else
    noncopyable() { }
private:
    noncopyable(noncopyable const&){}
    void operator=(noncopyable const&){}
#endif // __cplusplus >= 201103L
};

/**
 * If you don't like the inhritance approach,
 * can use the macro.
 *
 * This will disable implicit move operation
 * User need explicit declare and define it
 * (=default is also ok but it isn't recommended)
 */
#if __cplusplus >= 201103L
# define DISABLE_EVIL_COPYABLE(class_name) \
  class_name(class_name const& ) = delete; \
  class_name& operator=(class_name const&) = delete;
#else
# define DISABLE_EVIL_COPYABLE(class_name) \
 private:
  class_name(class_name const& ) {} \
  class_name& operator=(class_name const&) {}
#endif // __cplusplus >= 201103L

} // namespace kanon

#endif //KANON_NONCOPYABLE_H

