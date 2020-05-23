
#include "xml.h"

extern pthread_key_t xml_key;

xml_node_t *xml_new_xml(const char *version, const char *encoding, const char *standalone)
{
	char element[1024];
	memset(element, 0 ,sizeof(element));
	if (standalone)
		sprintf(element, "?xml version=\"%s\" encoding=\"%s\" standalone = \"%s\"?", version ? version : "1.0", encoding ? encoding : "GB2312", \
			standalone ? standalone : "true");	
	else
		sprintf(element, "?xml version=\"%s\" encoding=\"%s\"?", version ? version : "1.0", encoding ? encoding : "utf-8");
	return xml_new_element(NULL, element);
}

xml_node_t *xml_new_element(xml_node_t *parent, const char  *name)
{
	xml_node_t	*node = NULL;
	if (!name)
	{
		printf("xml_new_element name = null...\n");
		return NULL;
	}		

	if ((node = xml_new(parent))) node->element.name = strdup(name);
	return (node);
}

xml_node_t *xml_new(xml_node_t *parent)
{
	xml_node_t	*node = NULL;
	if ((node = calloc(1, sizeof(xml_node_t))) == NULL)
	{
		printf("xml_new calloc分配内存失败...\n");
		return NULL;
	}		
	node->ref_count = 1;
	if (parent) xml_add(parent, node);
	return node;
}
void xml_add(xml_node_t *parent, xml_node_t *node)
{
	pthread_mutex_lock(&mutex1);	
	node->parent = parent;//
	HH("\n\n!!!!!!!!!!!!!xml_add: node = %p node->parent = %p\n\n", node, node->parent);
	if (!parent) return;
	if (parent->last_child)
	{
		parent->last_child->next = node;
	}
	else
	{
		parent->child = node;		
	}
	parent->last_child = node;
	pthread_mutex_unlock(&mutex1);
}

void xml_set_attr(xml_node_t *node, const char  *name, const char  *value)//设置属性
{
	char *valuec = NULL;
	if (!node || !name) return;
	if (value) valuec = strdup(value);
	else valuec = NULL;
	if (set_attr(node, name, valuec)) free(valuec);
}

int set_attr(xml_node_t *node, const char *name, char *value)
{
	HH("function[set_attr]\n");
	int	i = 0;
	xml_attr_t *attr;
	pthread_mutex_lock(&mutex2);
	for (i = node->element.num_attrs, attr = node->element.attrs; i > 0; i--, attr++)
	{
		if (!strcmp(attr->name, name))
		{
			if (attr->value) free(attr->value);
			attr->value = value;
			return 0;
		}
	}
	if (node->element.num_attrs == 0)
		attr = malloc(sizeof(xml_attr_t));
	else
		attr = realloc(node->element.attrs,(node->element.num_attrs + 1) * sizeof(xml_attr_t));
	if (!attr) return -1;
	node->element.attrs = attr;
	attr += node->element.num_attrs;//找到新申请节点的空间，填充name和value
	if ((attr->name = strdup(name)) == NULL) return -1;
	attr->value = value;
	node->element.num_attrs++;
	pthread_mutex_unlock(&mutex2);
	return 0;
}

xml_buffer_t *xml_global(void)
{
	xml_buffer_t *global = NULL;
	HH("function[xml_global]\n");
	pthread_once(&xml_key_once, xml_init);
	if ((global = (xml_buffer_t *)pthread_getspecific(xml_key)) != NULL)
	{
		printf("function[xml_global] free before...\n");
		if (global->xml_buffer) free(global->xml_buffer);
		free(global);
	}
	//if ((global = (xml_buffer_t *)pthread_getspecific(xml_key)) == NULL) //一开始是这样,多线程没问题,但是在一个线程中多次调用出现问题
	global = (xml_buffer_t *)calloc(1, sizeof(xml_buffer_t));
	HH("function[xml_global]) global = %p\n", global);
	if (!global) return NULL;
	global->xml_buffer = (char *)calloc(1, LEN);
	HH(".. function[xml_global]) global = %s\n", global->xml_buffer);
	if (!global->xml_buffer)
	{
		printf("xml_global calloc failed...\n");
		return NULL;
	}
	global->buffer_len = LEN;
	global->available = LEN;
	pthread_setspecific(xml_key, global);
	HH("function[xml_global] end...\n");
	return global;
}
//xml_buffer_t *xml_save_buffer(xml_node_t *node)
char *xml_save_buffer(xml_node_t *node)//保存字符串动态申请的缓冲区2种释放方式
{							//1.满足主线程使用pthread_exit(0)退出,普通线程在进程退出之前结束便可自动释放 2.手动调用xml_free_buffer()释放
	xml_buffer_t *global = NULL;		
	global = xml_global();
	if (!global)
	{
		printf("分配内存失败...\n");
		return NULL;
	}
	if(xml_write_node(node) == -1) return NULL;	
	xml_buffer_t *pp = (xml_buffer_t *)pthread_getspecific(xml_key);
	HH("string:%s\n", pp->xml_buffer);
	HH("function[xml_save_buffer]) global = %p\n", pp);
	//return global;
	return global->xml_buffer;
}
static int xml_write_node(xml_node_t *node)
{
	xml_node_t	*current = NULL, *next = NULL;
	int	i = 0;
	xml_attr_t	*attr = NULL;
	char s[255];
	xml_buffer_t *global = NULL;
	global = pthread_getspecific(xml_key);
	printf("AAA global->xml_buffer:%s\n", global->xml_buffer);
	for (current = node; current; current = next)
	{
		if(xml_puts("<") < 0) return -1;
		if(xml_puts(current->element.name) < 0) return -1;//写元素		
		for (i = current->element.num_attrs, attr = current->element.attrs; i > 0; i--, attr++)//写属性字段
		{
			if(xml_puts(" ") < 0) return -1;
			if(xml_puts(attr->name) < 0) return -1;
			if (attr->value)
			{
				if(xml_puts("=") < 0) return -1;
				if(xml_puts("\"") < 0) return -1;
				if(xml_puts(attr->value) < 0) return -1;
				if(xml_puts("\"") < 0) return -1;
			}
		}
		if (current->child)
		{
			if(xml_puts(">") < 0) return -1;
		}
		else if (current->element.name[0] == '?')
		{
			if(xml_puts(">") < 0) return -1;
		}
		else
		{
			if(xml_puts(" ") < 0) return -1;
			if(xml_puts("/") < 0) return -1;
			if(xml_puts(">") < 0) return -1;
		}
		if ((next = current->child) == NULL)
		{
			HH("child null ...\n");
			if (current == node)
			{
				next = NULL;
			}
			else
			{
				HH("child null else...\n");
				while ((next = current->next) == NULL)
				{
					if (current == node || !current->parent)
						break;
					current = current->parent;
					if (current->element.name[0] != '?')
					{
						if(xml_puts("<") < 0) return -1;
						if(xml_puts("/") < 0) return -1;
						if(xml_puts(current->element.name) < 0) return -1;
						if(xml_puts(">") < 0) return -1;
					}
					if (current == node)
						break;
				}
			}
		}
	}
	if (xml_puts("\0") < 0) return -1;
	/*xml_buffer_t *global = NULL;
	global = pthread_getspecific(xml_key);
	len1 = strlen(global->xml_buffer);
	global->xml_buffer[len1] = '\0';*/
	return 0;
}

int xml_puts(const char *str)//连接字符串到缓冲区,调整缓冲区的大小
{
	char *p = NULL;
	int len = 0, ava_len = 0, length = 0;
	length = strlen(str);
	xml_buffer_t *global = NULL; 
	global = pthread_getspecific(xml_key);
	while (global->available < length)
	{
		p = (char *)realloc(global->xml_buffer, global->buffer_len * 2);
		if (p == NULL)
		{
			free(global->xml_buffer);
			free(global);
			return -1;
		}
		global->xml_buffer = p;
		global->available += global->buffer_len;
		global->buffer_len *= 2;		
	}
	//HH("function[xml_puts] %s\n",str);
	strcat(global->xml_buffer, str);
	global->available -= length;
	return 0;
}


static void xml_init(void)
{
	pthread_key_create(&xml_key, xml_destructor);
}
static void xml_destructor(void *g)//线程结束自动调用xml_destructor释放空间
{
	xml_buffer_t *ptr = NULL;
	if (!g) return;
	printf("xml_destructor free...\n");
	ptr = (xml_buffer_t *)g;
	if (ptr/* && ptr->xml_buffer && *ptr->xml_buffer*/)
	{
		HH("function[xml_destructor] ptr->xml_buffer:\n%s\n", ptr->xml_buffer);
		if (ptr->xml_buffer)
		{
			free(ptr->xml_buffer);
			ptr->xml_buffer = NULL;
		}			
		free(ptr);
		ptr = NULL;
		printf("free successful..\n");
	}			
}

void xml_del_key()//这个函数一调用保存字符串的空间就不能自动释放了
{
	pthread_key_delete(xml_key);
}

void xml_del_xml(xml_node_t *node)
{
	xml_node_t	*current = NULL, *next = NULL;
	pthread_mutex_lock(&mutex3);
	if (!node) return;
	xml_remove(node);
	for (current = node->child; current; current = next)
	{
		if ((next = current->child) != NULL)
		{
			current->child = NULL;
			continue;
		}

		if ((next = current->next) == NULL)
		{
			if ((next = current->parent) == node)
				next = NULL;
		}
		xml_free(current);
	}
	xml_free(node);
	pthread_mutex_unlock(&mutex3);
}

void xml_remove(xml_node_t *node)
{
	if (!node || !node->parent) return;
	if (node->prev)
		node->prev->next = node->next;
	else
		node->parent->child = node->next;
	if (node->next)
		node->next->prev = node->prev;
	else
		node->parent->last_child = node->prev;
	node->parent = NULL;
	node->prev = NULL;
	node->next = NULL;
}

static void xml_free(xml_node_t *node)
{
	int i = 0;
	if (node->element.name) HH("free node:node->element.name:%s\n", node->element.name);
	if (node->element.name)
	{
		free(node->element.name);
		node->element.name = NULL;
	}
	if (node->element.num_attrs)
	{
		for (i = 0; i < node->element.num_attrs; i++)
		{
			HH("node->element.attrs[%d].name:%s\n",i, node->element.attrs[i].name);
			if (node->element.attrs[i].name)
			{
				free(node->element.attrs[i].name);
				node->element.attrs[i].name = NULL;
			}
				
			HH("node->element.attrs[%d].value:%s\n", i, node->element.attrs[i].name);
			if (node->element.attrs[i].value)
			{
				free(node->element.attrs[i].value);
				node->element.attrs[i].value = NULL;
			}
				
		}
		free(node->element.attrs);
		node->element.attrs = NULL;
	}
	HH("node:%p\n",node);
	free(node);
	node = NULL;
}

void xml_free_buffer()
{
	printf("xml_free_buffer free...\n");
	xml_buffer_t *p = pthread_getspecific(xml_key);
	if (p)
	{
		printf("function[xml_free_buffer] p->xml_buffer:\n%s\n",p->xml_buffer);
		if (p->xml_buffer)
		{
			free(p->xml_buffer);
			p->xml_buffer = NULL;
		}			
		free(p);
		p = NULL;
		printf("free successful...\n");
	}	
	//xml_del_key();//这个函数不能调用,要注释掉
}

char *xml_trans_encoding(char *tocode, char *fromcode)//编码转换
{
	
	size_t len_in = 0, len_out = 0;//这里类型搞错了会出现问题
	char *in = NULL,*out = NULL, *pin = NULL, *pout = NULL, *ptr = NULL;
	iconv_t conv;
	xml_buffer_t *global = pthread_getspecific(xml_key);
	HH("function[xml_trans_encoding]\n");
	HH("\n..global->xml_buffer = %s\n", global->xml_buffer);

	if (!global || !global->xml_buffer)
	{
		printf("xml_trans_encoding buffer is null...\n");
		return NULL;
	}
	len_in = strlen(global->xml_buffer);
	HH("aalen_in = %ld \n", len_in);
	in = (char *)malloc(len_in);
	out = (char *)malloc(len_in * 4);
	memset(in, 0, len_in);
	strcpy(in, global->xml_buffer);//直接转换总是失败,通过这种方式可以转换
	len_out = (size_t)(len_in * 4);
	HH("len_in = %ld in= %s\n",len_in, in);
	pin = in;
	pout = out;
	if((conv = iconv_open(tocode, fromcode)) == (iconv_t)-1) return NULL;
	iconv(conv, &pin, &len_in, &pout, &len_out);
	iconv_close(conv);
	HH("##3\n");
	HH("\n\n..out=%s\n", out);
	/*if (strlen(out) > len_in)
	{
		if ((ptr = realloc(global->xml_buffer, strlen(out) + 1)) != NULL)
			global->xml_buffer = ptr;
		else
		{
			printf("xml_trans_encoding realloc failed...\n");
			return NULL;
		}
			
	}*/
	free(global->xml_buffer);
	global->xml_buffer = out;
	HH("\n\n..p = %p\nglobal->xml_buffer=%s\n", global->xml_buffer, global->xml_buffer);
	free(in);
	//free(out);
	return global->xml_buffer;
}
//----------------------------------

char *xml_trans_encoding2(char *tocode, char *fromcode, char *string)//编码转换
{
	size_t len_in = 0, len_out = 0;
	char *out = NULL, *pin = NULL, *pout = NULL;
	iconv_t conv;
	len_in = strlen(string);
	HH("aalen_in = %ld \n", len_in);
	out = (char *)malloc(len_in * 4);	
	len_out = (size_t)(len_in * 4);
	memset(out, 0, len_out);
	pin = string;
	pout = out;
	if ((conv = iconv_open(tocode, fromcode)) == (iconv_t)-1) return NULL;
	iconv(conv, &pin, &len_in, &pout, &len_out);
	iconv_close(conv);
	return out;
}

xml_node_t *xml_parse_string(xml_node_t *top, char *string)
{
	char *ptr = NULL, *p = NULL;
	char **pp = NULL;
	char *buffer = NULL;
	unsigned size = 0;
	xml_node_t *node = NULL, *parent = NULL, *first = NULL;
	parent = top;
	int i = 0;
	if (!string) return NULL;
	if (*(ptr = string) != '<')
	{
		printf("格式错误...\n");
		return NULL;
	}
	HH("before parse ptr = %s\n", ptr);
	while (*ptr != '\0')
	{
		i = 0;
		if (*ptr == '<')//
		{
			ptr++;
			if (*ptr == '<')
			{
				printf("格式错误...\n");
				return NULL;
			}
			for (;/**ptr != '>' && !xml_isspace(*ptr) && buffer[0] == '?'*/; ptr++)
			{
				HH("for loop...\n");
				if (*ptr == '>' && (ptr[1] != '<' && !xml_isspace(ptr[1]) && ptr[1] != '\0'))//格式错误返回null
				{
					printf("格式错误... ptr[1] = %d\n",ptr[1]);
					return NULL;
				}
				if (*ptr == '>' || xml_isspace(*ptr))
				{
					*p = '\0';//这里一开始没加导致解析中文出错
					break;
				}
				if (p > buffer && *ptr == '/')
				{
					HH("vv*ptr = %c buffer[0] = %c\n", *ptr, buffer[0]);
					break;
				}
				if (xml_put_c(&buffer, &size, &p, *ptr) == -1)
				{
					goto error;
				}
				if (buffer[0] == '?') break;//头声明
			}
			HH("\nafter for loop buffer:%s\n", buffer);

			HH("*ptr = %c\n", *ptr);
			if (buffer[0] == '?')//解析头声明
			{
				ptr++;
				for (;; ptr++)
				{
					if (*ptr == '>')
					{
						*p = '\0';
						break;
					}
					xml_put_c(&buffer, &size, &p, *ptr);
					HH("buffer[0] = \'?\' buffer = %s\n", buffer);
				}
				HH("\n\nQQQQ buffer = %s \n\n", buffer);
				node = xml_new_element(parent, buffer);
				memset(buffer, 0, size);
				p = buffer;
				if (node)
				{
					HH("top...\n");
					if (!parent) parent = node;
					if (!first) first = node;
					top = first;
					HH("\n\ntou ?xml=%p\n\n",node);
				}
			}
			else if (buffer[0] == '/')
			{
				p = buffer;
				HH("\n\n#######buffer[0] == \'/\' %p %p\n\n\n",parent,parent->parent);
				for (;; ptr++)
				{
					if (*ptr == '>')
					{
						memset(buffer, 0, sizeof(buffer));
						parent = parent->parent;
						HH("\n\n@@##@@@buffer[0] == \'/\' %p\n\n\n", parent);
						break;
					}
					xml_put_c(&buffer, &size, &p, *ptr);
				}

				HH("\n\nafter #######buffer[0] == \'/\' %p  *ptr = %c\n\n\n", parent, *ptr);
			}
			else
			{
				ZZ("\nAAAAelse buffer = %s\n", buffer);
				if ((node = xml_new_element(parent, buffer)))
				{
					HH("*p = %c *(p-1) = %c\n", *p, *(p - 1));
					memset(buffer, 0, size);
					parent = node;//这里要和下面统一
					p = buffer;
				}
				else {
					HH("function[mxmlNewElement]...\n");
					goto error;
				}
				if (xml_isspace(*ptr))
				{
					pp = &ptr;
					xml_parse_element(node, pp);
				}
				HH("!!!!\n");
				HH("!!ptr= %p\n", ptr);
				HH("*ptr = %c\n", *ptr);
				if (*ptr == '>')
				{

					if (ptr[-1] != '/')
					{
						HH("*********ifb  has son  ptr[-1] = %c  parent = %p node = %p\n", ptr[-1], parent,node);
						parent = node;//有儿子
						HH("*********if  has son  ptr[-1] = %c  parent = %p node = %p\n", ptr[-1], parent,node);
					}
					else
					{
						HH("*********else noson ptr[-1] = %c\n\n", ptr[-1]);
						parent = parent->parent;//没有儿子	
					}
				}
				HH("\nOUT else %s\n\n",node->element.name);
			}
		}
		ptr++;
	}
	HH("complete parse... *ptr = %d\n", *ptr);
	free(buffer);
	return top;
error:
	free(buffer);
	printf("error...\n");
	return NULL;
}

int xml_put_c(char **buffer, unsigned *size, char **p, int ch)
{
	HH("function[xml_put_c]\n");
	char *ptr = NULL;
	if (*buffer == NULL)
	{
		HH("if...\n");
		if ((*buffer = (char *)malloc(64)) == NULL) return -1;
		*size = 64;
		*p = *buffer;
		HH("buffer = %p *p = %p\n", *buffer, *p);
		//if (ch == -1) return 0;
	}
	else if ((*p - *buffer) == (*size - 1))
	{
		HH("else...\n");
		if (*size < 1024)
			*size *= 2;
		else
			*size += 1024;
		ptr = realloc(*buffer, *size);
		if (ptr == NULL)
		{
			HH("realloc 分配内存失败\n");
			return -1;
		}
		*buffer = ptr;
	}
	**p = ch;
	(*p)++;
	HH("*p = %p buffer = %p ch = %c\n", *p, *buffer, ch);
	HH("11\n");
	return ch;
}

int xml_isspace(int ch)
{
	return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
}

int xml_parse_element(xml_node_t *node, char **pp)
{
	HH("function[xml_parse_element]\n");
	char name[128] = { 0 }, value[128] = { 0 };
	int name_flag = 0, value_flag = 0;
	int i = 0, j = 0;
	name_flag = 1;
	for (; **pp != '\0' && **pp != '>'; (*pp)++)
	{
		if (xml_isspace(**pp))
		{
			HH("if (*p)[1] = %c\n", (*pp)[1]);
			if (xml_isspace((*pp)[1]) || (*pp)[1] == '/') continue;
			memset(name, 0, sizeof(name));
			name_flag = 1;
			value_flag = 0;
			i = 0;
			continue;
		}
		if (**pp == '=')
		{
			name[i] = '\0';
			HH("name = %s\n", name);
			if ((*pp)[1] == '\"')
			{
				memset(value, 0, sizeof(value));
				(*pp)++;
				name_flag = 0;
				value_flag = 1;
				i = 0;
				continue;
			}
			else
			{
				printf("格式错误,缺少'\"'...\n");
				return -1;
			}
		}
		if (name_flag)
		{
			name[i++] = **pp;
		}
		if (value_flag)
		{
			//HH("q %c i = %d\n", *ptr, i);
			value[i++] = **pp;
			if (**pp == '\"')
			{
				value[i - 1] = '\0';
				HH("value = %s\n", value);
				xml_set_attr(node, name, value);
			}
		}
	}
	HH("after parse **pp = %c\n", **pp);
	HH("PTR = %p\n", *pp);
	return 0;
}


xml_node_t *xml_walk_next(xml_node_t *node, xml_node_t *top)
{
	if (!node) return NULL;
	else if (node->child) return node->child;
	else if (node == top) return NULL;
	else if (node->next) return node->next;
	else if (node->parent && node->parent != top)
	{
		node = node->parent;
		while (!node->next)
			if (node->parent == top || !node->parent)
				return NULL;
			else
				node = node->parent;
		return node->next;
	}
	else return NULL;
}
xml_node_t *xml_find_element(xml_node_t *node, xml_node_t *top, const char  *element, const char  *attr, const char  *value)
{
	char *temp = NULL;
	if (!node || !top || (!attr && value)) return NULL;
	node = xml_walk_next(node, top);
	while (node != NULL)
	{
		ZZ("element = %s\n", element);
		ZZ("node->element.name = %s\n", node->element.name);
		if (!strcmp(node->element.name, element))
		{
			if (!attr) return (node);
			if ((temp = xml_get_attr(node, attr)) != NULL)
			{
				if (!value || !strcmp(value, temp)) return node;
			}
		}
		node = xml_walk_next(node, top);
	}
	return NULL;
}


char *xml_get_attr(xml_node_t *node, const char *name)
{
	int	i = 0;
	xml_attr_t *attr = NULL;

	if (!node || !name) return NULL;
	for (i = node->element.num_attrs, attr = node->element.attrs; i > 0; i--, attr++)
	{
		if (!strcmp(attr->name, name))
		{
			return attr->value;
		}
	}
	return NULL;
}
