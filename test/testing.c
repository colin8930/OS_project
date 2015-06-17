#include <stdio.h>
#include <string.h>

FILE *fp;

void test(char c)
{
	fp = fopen("/dev/test", "w+");
	if(fp == NULL){
		printf("can't open device\n");
		return ;
	}
	char input[1];
	input[0] = c;
	fwrite(input, sizeof(input), 1, fp);
	char output[1];
	fread(output, sizeof(output), 1, fp);
	printf("%c", output[0]);
	fclose(fp);
	return;
}

int main()
{
	test('h');
	test('e');
	test('l');
	test('l');
	test('o');
	test('\n');
	return 0;
}
