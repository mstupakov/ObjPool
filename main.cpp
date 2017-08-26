#include <iostream>
#include <vector>

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

  return 0;
}
