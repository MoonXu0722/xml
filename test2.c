#include "xml.h"

int main()
{
	char string[4096];
	char *pp = NULL;
	FILE *fp = NULL;
	char *ptr = NULL;
	xml_node_t *p = NULL;
	memset(string, 0, sizeof(string));
	fp = fopen("a.xml","rb");
	if(!fp) return -1;
	fread(string, 1, 4096, fp);
	fclose(fp);
	pp = xml_trans_encoding2("UTF-8", "GB2312", string);
	if(pp) printf("pp:\n%s\n",pp);
	xml_node_t *xml = xml_parse_string(NULL, pp);
	free(pp);
	if(!xml) return -2;

	xml_node_t *set = xml_find_element(xml, xml, "AlarmSearchPSet", NULL, NULL);
	if(set) ptr = xml_get_attr(set, "AudioPID");
	if(ptr) printf("AudioPID = \"%s\"\n",ptr);
	p = xml_find_element(set, xml, "AlarmSearchP", NULL, NULL);
	while(p)
	{
		ptr = xml_get_attr(p, "Desc");
		if(ptr) printf("Desc = \"%s\"\n",ptr);
		p = xml_find_element(p, xml, "AlarmSearchP", NULL, NULL);
	}
	return 0;
}
