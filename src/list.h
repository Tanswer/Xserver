#ifndef LIST_H
#define LIST_H

#include <stddef.h>

typedef struct list_head {
    struct list_head *prev, *next;
}list_head;

#define INIT_LIST_HEAD(ptr) do {\
    struct list_head *_ptr = (struct list_head *)ptr;\
    (_ptr)->next = (_ptr);  (_ptr->prev) = _ptr;     \
}while(0);

static inline void __list_add(struct list_head *_new, struct list_head *prev, struct list_head *next){
    _new -> next = next;
    next -> prev = _new;
    prev -> next = _new;
    _new -> prev = prev;
}

/* 头插法 */
static inline void list_add(struct list_head *_new, struct list_head *head){
    __list_add(_new, head, head->next);
}

/* 尾插法 */
static inline void list_add_tail(struct list_head *_new, struct list_head *head){
    __list_add(_new, head->prev, head);
}


static inline void __list_del(struct list_head *prev, struct list_head *next){
    prev -> next = next;
    next -> prev = prev;
}


static inline void list_del(struct list_head *entry){
    __list_del(entry -> prev, entry -> next);
}

static inline int list_empty(struct list_head *head){
    return (head->next == head) && (head->prev == head);
}

/* 获取 MEMBER 在 TYPE 中的偏移 */
/*
 * 1. ((TYPE *)0)  将 0 转换为 TYPE 类型指针
 * 2. ((TYPE *)0)->MEMBER 访问结构中的成员 MEMBER
 * 3. &((TYPE *)0)->MEMBER 取出数据成员的地址
 * 4. (size_t)(&((TYPE *)0)->MEMBER) 
 */
//#define offsetof(TYPE, MEMBER) ((size_t)&((TYPE *)0)->MEMBER)


/* 由 member 成员的地址 ptr 得到包含 member 成员的 type 的地址 */
/* 如何通过结构中某个变量的地址获取结构本身的指针 */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type, member) );})

#define list_entry(ptr, type, member)   \
    container_of(ptr, type, member) 

/* 在这坑了，忘了给 head 加括号，一直保存，郁闷了快两个小时。。。*/
#define list_for_each(pos, head) \
    for(pos = (head) -> next; pos != (head); pos = pos -> next)


//static inline 

#endif
