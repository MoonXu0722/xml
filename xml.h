#ifndef XML_H__
#define XML_H__
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <iconv.h>

#define ZZ //printf
#define HH //printf
#define LEN 1024
pthread_key_t	xml_key;
static pthread_once_t	xml_key_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;//添加节点的锁
static pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;//设置属性的锁
static pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;//删除节点的锁
typedef struct
{
	char *xml_buffer;
	int buffer_len;
	int available;
}xml_buffer_t;//线程私有数据

typedef struct xml_attr
{
	char *name;
	char *value;
}xml_attr_t;

typedef struct xml_element
{
	char *name;
	int	num_attrs;
	xml_attr_t *attrs;
}xml_element_t;

typedef struct xml_node
{
	xml_element_t element;
	struct xml_node *prev;
	struct xml_node *next;
	struct xml_node *parent;
	struct xml_node *child;
	struct xml_node *last_child;
	int ref_count;
}xml_node_t;


typedef int (*xml_put_char)(int ch, void *p);
xml_node_t *xml_new_xml(const char *version, const char *encoding, const char *standalone);
xml_node_t *xml_new_element(xml_node_t *parent, const char  *name);
xml_node_t *xml_new(xml_node_t *parent);
void xml_add(xml_node_t *parent, xml_node_t *node);
void xml_set_attr(xml_node_t *node, const char  *name, const char  *value);
int set_attr(xml_node_t *node, const char  *name, char *value);
//xml_buffer_t *xml_save_buffer(xml_node_t *node);
char *xml_save_buffer(xml_node_t *node);
static int xml_write_node(xml_node_t *node);
static void xml_init(void);
void xml_del_key();
static void xml_destructor(void *g);
xml_buffer_t *xml_global(void);
int xml_puts(const char *str);
static void xml_free(xml_node_t *node);
void xml_free_buffer();//释放保存字符串的缓冲区
void xml_remove(xml_node_t *node);
void xml_del_xml(xml_node_t *node);
char *xml_trans_encoding(char *tocode, char *fromcode);
char *xml_trans_encoding2(char *tocode, char *fromcode,char *string);

extern  xml_node_t *xml_parse_string(xml_node_t *top, char *string);
extern int xml_put_c(char **buffer, unsigned *size, char **p, int ch);
extern int xml_isspace(int ch);
extern int xml_parse_element(xml_node_t *node, char **ptr);
extern xml_node_t *xml_walk_next(xml_node_t *node, xml_node_t *top);
extern xml_node_t *xml_find_element(xml_node_t *node, xml_node_t *top, const char  *element, const char  *attr, const char  *value);
extern char *xml_get_attr(xml_node_t *node, const char  *name);

#endif //XML_H