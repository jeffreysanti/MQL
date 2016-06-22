#include "utils.h"


char *dupl(const char *s, const char *end){
	char *res = malloc(end - s + 1);
	memcpy(res, s, end-s);
	res[end - s] = 0;
	return res;
}

char *dup(const char *s){
	return dupl(s, s+strlen(s));
}

char *trim(char *str){
	char *s, *t;
	for (s = str; whitespace(*s); s++) ;
	
	if (*s == 0)
		return s;

	t = s + strlen(s) - 1;
	while (t > s && whitespace(*t))
		t--;
	*++t = '\0';

	return s;
}

char *strrev(char *str){
	unsigned int i = strlen(str)-1;
	unsigned int j=0;	
	while(i>j){
		char ch = str[i];
		str[i]= str[j];
		str[j] = ch;
		i--;
		j++;
	}
	return str;
}


