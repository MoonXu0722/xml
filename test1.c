#include "xml.h"
typedef struct
{
	xml_node_t *msg;
	xml_node_t *ret;
	xml_node_t *set;
	xml_node_t *p1;
	xml_node_t *p2;
	xml_node_t *p3;
	xml_node_t *p4;
	xml_node_t *p5;
	xml_node_t *p6;
}param;
void func1(void *p)
{
	param *g = (param *)p;
	xml_set_attr(g->msg, "Version", "2.3");
	xml_set_attr(g->msg, "Type", "MonUp");
	xml_set_attr(g->msg, "SrcCode", "043311");
	xml_set_attr(g->msg, "ReplyID", "-1");
	xml_set_attr(g->ret, "Type", "AlarmSearchPSet");
	xml_set_attr(g->ret, "value", "0");
	xml_set_attr(g->set, "Freq", "251000");
	xml_set_attr(g->set, "AudioPID", "1621");
	xml_set_attr(g->p1, "Value", "1");
	xml_set_attr(g->p1, "Type", "32");
	xml_set_attr(g->p4, "Desc", "无载波");
	xml_set_attr(g->p5, "Desc", "无声音");
	xml_set_attr(g->p6, "Desc", "黑场");

}
void func2(void *p)
{
	param *g = (param *)p;
	xml_set_attr(g->msg, "MsgID", "462723");
	xml_set_attr(g->msg, "DstCode", "0002");
	xml_set_attr(g->msg, "DateTime", "2016-05-20");
	xml_set_attr(g->ret, "Desc", "成功");
	xml_set_attr(g->set, "VideoPID", "1611");
	xml_set_attr(g->set, "ServiceID", "1601");
	xml_set_attr(g->set, "Index", "1");
	xml_set_attr(g->p1, "Time", "2016-05-20 10:00:01");
	xml_set_attr(g->p1, "Desc", "图像静止");
	xml_set_attr(g->p2, "Desc", "无伴音");
	xml_set_attr(g->p3, "Desc", "彩条");
}

int main()
{
	pthread_t pid[2];
	xml_node_t *xml = xml_new_xml(NULL, "GB2312", "yes");
	xml_node_t *msg = xml_new_element(xml, "Msg");
	xml_node_t *ret = xml_new_element(msg, "Return");
	xml_node_t *info = xml_new_element(ret, "ReturnIofo");
	xml_node_t *set = xml_new_element(info, "AlarmSearchPSet");
	xml_node_t *p1 = xml_new_element(set, "AlarmSearchP");
	xml_node_t *p2 = xml_new_element(set, "AlarmSearchP");
	xml_node_t *p3 = xml_new_element(set, "AlarmSearchP");
	xml_node_t *p4 = xml_new_element(set, "AlarmSearchP");
	xml_node_t *p5 = xml_new_element(set, "AlarmSearchP");
	xml_node_t *p6 = xml_new_element(set, "AlarmSearchP");
	param para = {msg, ret, set, p1, p2, p3, p4, p5, p6};
	pthread_create(&pid[0], NULL, (void *)func1, &para);
	pthread_create(&pid[1], NULL, (void *)func2, &para);
	pthread_join(pid[0], NULL);
	pthread_join(pid[1], NULL);
	char *str = xml_save_buffer(xml);
	printf("str = %s\n",str);
	str = xml_trans_encoding("GB2312", "UTF-8");
	FILE *fp = fopen("a.xml", "wb");
	if(!fp) return -1;
	fwrite(str, 1, strlen(str), fp);
	fclose(fp);
	str = NULL;
	pthread_exit(0);
	//xml_free_buffer();
	//return 0;
}
