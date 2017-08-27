#include <iostream>
#include <vector>
#include <string>
#include <memory>

template<typename T>
class ObjPool {
  public:
    ObjPool(unsigned size) {
      m_head = new Node(0);

      for (unsigned i = 1; i < size; ++i) {
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
#endif

    void free(T* obj) {
      obj->~T();
      m_head = new (obj) Node(m_head);
    }

  private:
    ObjPool(const ObjPool&);
    ObjPool& operator=(const ObjPool&);

    union Node {
      char obj[sizeof(T)];
      Node* next;

      Node(Node* node) : next(node) {}
    };

    Node* m_head;
};

struct A {
  A() {
    std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }

 ~A() {
    std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }

  A(const A&) {
    std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }

  A& operator=(const A&) {
    std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
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

  B& operator=(const B&) {
    std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }

  void method() {
    std::cout << __PRETTY_FUNCTION__ << " This: " << this << std::endl;
  }
};

int main() {
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

  u_ptr1->method();
  u_ptr2->method();
  u_ptr3->method();

  return 0;
}
