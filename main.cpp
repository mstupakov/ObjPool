#include <valgrind/callgrind.h>

#include <cstring>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <functional>


extern void f();
extern void f2(void*);

template<typename T>
class AA {
  T* m_pointer;

  public:
    template<typename ...Args>
    AA(Args... args) {
    
    }
};

template<typename T, bool S = false>
class ObjPool {
  public:
    ObjPool(unsigned size) : m_head(0) {
      for (unsigned i = 0; i < size; ++i) {
        m_head = new Node(m_head);
      }
    }

   ~ObjPool() {
      while (m_head) {
        Node* next = m_head->next;

        delete m_head;
        m_head = next;
      }
    }

    T* get() {
      if (!m_head) {
        return 0;
      }

      Node* next = m_head->next;
      T* obj = new (m_head) T;
      m_head = next;
      return obj;
    }

#if __cplusplus >= 201103L
    template<typename... Args>
    T* get(Args&&... args) {
      if (!m_head) {
        return 0;
      }

      Node* next = m_head->next;
      T* obj = new (m_head) T(std::forward<Args> (args)...);
      m_head = next;
      return obj;
    }

    using ObjUPtr = std::unique_ptr<T, std::function<void(T*)> >;

    template<typename... Args>
    ObjUPtr get_unique(Args&&... args) {
      return ObjUPtr(get(std::forward<Args> (args)...),
          [this] (T* obj) { this->free(obj); } );
    }

    using ObjSPtr = std::shared_ptr<T>;

    template<typename... Args>
    ObjSPtr get_shared(Args&&... args) {
      return ObjSPtr(get(std::forward<Args> (args)...),
          [this] (T* obj) { this->free(obj); } );
    }

    
    using ObjMPtr = AA<T>;

    template<typename... Args>
    ObjMPtr get_maks(Args&&... args) {
      return ObjMPtr(get(std::forward<Args> (args)...),
          [this] (T* obj) { this->free(obj); } );
    }
#endif

    void free(T* obj) {
      obj->~T();
      m_head = new (obj) Node(m_head);
    }

  private:
    ObjPool(const ObjPool&);
    ObjPool& operator=(const ObjPool&);

    union NodeU {
      char obj[sizeof(T)];
      NodeU* next;

      NodeU(NodeU* node) : next(node) {}
    };

    union NodeS {
      struct {
        char obj[sizeof(T)];
        unsigned refcount;
      };

      NodeS* next;
      NodeS(NodeS* node) : next(node) {}
    };

    using Node = typename std::conditional<!S, NodeU, NodeS>::type;

    Node* m_head;
};

struct A {
  A() {
    //std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }

 ~A() {
    //std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }

  A(const A&) {
    //std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }

  void operator=(const A&) {
    //std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }
};

struct B {
  B(int a, char b, const std::string& s) {
    std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
    std::cout << "   - a: " << a << std::endl;
    std::cout << "   - b: " << b << std::endl;
    std::cout << "   - s: " << s << std::endl;
  }

 ~B() {
    std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }

  B(const B&) {
    std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }

  void operator=(const B&) {
    std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }

  void method() {
    std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }
};

char a;
struct L2Frame {
  char payload[1500];
//  L2Frame() {a = payload[1499];}

  L2Frame() : payload() {
    f2(this);
  //  std::cout << __PRETTY_FUNCTION__ << std::endl;
  }

 ~L2Frame() {
   f2(this);
   // std::cout << __PRETTY_FUNCTION__ << std::endl;
  }
};

void test_shared(ObjPool<B>::ObjSPtr ptr) {
  ptr->method();
  std::cout << "Count: " << ptr.use_count() << std::endl;
}

int main() {
  delete new L2Frame;
  /*
  ObjPool<A> pool(5);

  A* obj_1 = pool.get();
  A* obj_2 = pool.get();
  A* obj_3 = pool.get();
  A* obj_4 = pool.get();

  std::cout << "--------------" << std::endl;

  pool.free(obj_2);

  A* obj_5 = pool.get();
  A* obj_6 = pool.get();

  pool.free(obj_1);
  pool.free(obj_4);
  pool.free(obj_3);
  pool.free(obj_5);
  pool.free(obj_6);

  ObjPool<std::vector<A> > pool_v(10);

  std::vector<A> *v = pool_v.get();
  v->push_back(A());

  pool_v.free(v);

  ObjPool<B> pool_b(5);
  B* b1 = pool_b.get(1, '$', "Hello!");

  B b2(1, '$', "Hello!");

  B* bb = pool_b.get(b2);
  pool_b.free(bb);
  pool_b.free(b1);

  ObjPool<B>::ObjUPtr u_ptr1 = pool_b.get_unique(b2);
  ObjPool<B>::ObjUPtr u_ptr2 = pool_b.get_unique(b2);
  ObjPool<B>::ObjUPtr u_ptr3 = pool_b.get_unique(2, '%', "World!");
  ObjPool<B>::ObjSPtr s_ptr4 = pool_b.get_shared(3, '&', "World!");

  u_ptr1->method();
  u_ptr2->method();
  u_ptr3->method();
  s_ptr4->method();

  test_shared(s_ptr4);

  ObjPool<L2Frame> pool_f(8);
  ObjPool<L2Frame>::ObjUPtr f_ptr1 = pool_f.get_unique();
  ObjPool<L2Frame>::ObjUPtr f_ptr2 = pool_f.get_unique();
  ObjPool<L2Frame>::ObjUPtr f_ptr3 = pool_f.get_unique();
  ObjPool<L2Frame>::ObjUPtr f_ptr4 = pool_f.get_unique();

  L2Frame* f_ptr6 = pool_f.get();
  pool_f.free(f_ptr6);

  std::cout << "Size: " << sizeof(*f_ptr1) << std::endl;

  memset(f_ptr1->payload, 0, sizeof(*f_ptr1));

  ObjPool<char[100]> pool_c(10);
  */

  ObjPool<L2Frame> pool_aa(10);
  ObjPool<L2Frame>::ObjUPtr p = pool_aa.get_unique();

  std::shared_ptr<L2Frame> sp2;


  {
    std::shared_ptr<L2Frame> sp(std::move(p));
    std::shared_ptr<L2Frame> sp1 = sp;
    sp2 = sp1;
  }

  std::cout << "Counter: " << std::endl;//<< sp.use_count() << std::endl;

#ifndef VV
#define VV 1
#endif

#if VV == 1
  ObjPool<L2Frame> pool_a(10);

  CALLGRIND_START_INSTRUMENTATION;

  L2Frame* a0 = pool_a.get();// f2(a0); 
  L2Frame* a1 = pool_a.get();// f2(a1);
  L2Frame* a2 = pool_a.get();// f2(a2);
  L2Frame* a3 = pool_a.get();// f2(a3);
  L2Frame* a4 = pool_a.get();// f2(a4);
  L2Frame* a5 = pool_a.get();// f2(a5);
  L2Frame* a6 = pool_a.get();// f2(a6);
  L2Frame* a7 = pool_a.get();// f2(a7);
  L2Frame* a8 = pool_a.get();// f2(a8);
  L2Frame* a9 = pool_a.get();// f2(a9);


  pool_a.free(a9);
  pool_a.free(a8);
  pool_a.free(a7);
  pool_a.free(a6);
  pool_a.free(a5);
  pool_a.free(a4);
  pool_a.free(a3);
  pool_a.free(a2);
  pool_a.free(a1);
  pool_a.free(a0);

  CALLGRIND_STOP_INSTRUMENTATION;
  CALLGRIND_DUMP_STATS;

#elif VV == 2

  ObjPool<L2Frame> pool_a(10);

  CALLGRIND_START_INSTRUMENTATION;

  {
    ObjPool<L2Frame>::ObjUPtr a0 = pool_a.get_unique();
    ObjPool<L2Frame>::ObjUPtr a1 = pool_a.get_unique();
    ObjPool<L2Frame>::ObjUPtr a2 = pool_a.get_unique();
    ObjPool<L2Frame>::ObjUPtr a3 = pool_a.get_unique();
    ObjPool<L2Frame>::ObjUPtr a4 = pool_a.get_unique();
    ObjPool<L2Frame>::ObjUPtr a5 = pool_a.get_unique();
    ObjPool<L2Frame>::ObjUPtr a6 = pool_a.get_unique();
    ObjPool<L2Frame>::ObjUPtr a7 = pool_a.get_unique();
    ObjPool<L2Frame>::ObjUPtr a8 = pool_a.get_unique();
    ObjPool<L2Frame>::ObjUPtr a9 = pool_a.get_unique();
  }

  CALLGRIND_STOP_INSTRUMENTATION;
  CALLGRIND_DUMP_STATS;

#elif VV == 3
  ObjPool<L2Frame> pool_a(10);

  CALLGRIND_START_INSTRUMENTATION;

  {
    ObjPool<L2Frame>::ObjSPtr a0 = pool_a.get_shared();
    ObjPool<L2Frame>::ObjSPtr a1 = pool_a.get_shared();
    ObjPool<L2Frame>::ObjSPtr a2 = pool_a.get_shared();
    ObjPool<L2Frame>::ObjSPtr a3 = pool_a.get_shared();
    ObjPool<L2Frame>::ObjSPtr a4 = pool_a.get_shared();
    ObjPool<L2Frame>::ObjSPtr a5 = pool_a.get_shared();
    ObjPool<L2Frame>::ObjSPtr a6 = pool_a.get_shared();
    ObjPool<L2Frame>::ObjSPtr a7 = pool_a.get_shared();
    ObjPool<L2Frame>::ObjSPtr a8 = pool_a.get_shared();
    ObjPool<L2Frame>::ObjSPtr a9 = pool_a.get_shared();
  }

  CALLGRIND_STOP_INSTRUMENTATION;
  CALLGRIND_DUMP_STATS;

#elif VV == 4

  CALLGRIND_START_INSTRUMENTATION;

  L2Frame* a0 = new L2Frame;// f2(a0);
  L2Frame* a1 = new L2Frame;// f2(a1);
  L2Frame* a2 = new L2Frame;// f2(a2);
  L2Frame* a3 = new L2Frame;// f2(a3);
  L2Frame* a4 = new L2Frame;// f2(a4);
  L2Frame* a5 = new L2Frame;// f2(a5);
  L2Frame* a6 = new L2Frame;// f2(a6);
  L2Frame* a7 = new L2Frame;// f2(a7);
  L2Frame* a8 = new L2Frame;// f2(a8);
  L2Frame* a9 = new L2Frame;// f2(a9);


  delete (a9);
  delete (a8);
  delete (a7);
  delete (a6);
  delete (a5);
  delete (a4);
  delete (a3);
  delete (a2);
  delete (a1);
  delete (a0);

  CALLGRIND_STOP_INSTRUMENTATION;
  CALLGRIND_DUMP_STATS;

#endif

//  ObjPool<L2Frame>::ObjMPtr pp = pool_a.get_maks();

  return 0;
}
