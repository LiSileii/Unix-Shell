#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;

int main()
{
	//1.��ʼ�������������ļ�

	//2.����ִ�У��������ѭ����
	lsh_loop();

	//3.��ֹ��ִ�йر�����

	return EXIT_SUCCESS;
}

void lsh_loop()
{
	char* line;
	char** args;
	int status;

	do { //Do-while ѭ���ڼ��״̬����ʱ������㣬��Ϊ�����ڼ�������ֵ֮ǰ��ִ��һ��
		printf("> ");
		//1.��ȡ����
		line = lsh_read_line();
		//2.����
		args = lsh_split_line(line);
		//3.ִ��
		status = lsh_execute(args); //����ִ�н���ж��Ƿ��˳�ѭ��

		//�ͷ�֮ǰΪline��args������ڴ�ռ䣬�����Ļ��Ͳ��ý�line��args������ѭ����
		free(line);
		free(args);
	} while (status);
}

#define LSH_RL_BUFSIZE 1024 //��ʼ�ռ��С
//constexpr auto LSH_RL_BUFSIZE = 1024; //Ҳ����constexpr
char* lsh_read_line()
{
	int bufsize = LSH_RL_BUFSIZE;
	int position = 0;
	char* buffer = (char *)malloc(sizeof(char) * bufsize); //�����ʼ�ڴ�ռ�
	int c; //������ַ�����Ϊint���Ͷ�����char����
		//��ΪEOF��һ������ֵ�������ַ���ֵ������뽫����ֵ��Ϊ�ж�����������Ҫʹ��int����

	if (!buffer)
	{
		fprintf(stderr, "lsh: allocation error\n"); //fprintf��ʽ�������һ�����ļ���
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		c = getchar(); //����һ���ַ�
		if (c == EOF || c == '\n') //��������ĩβ
		{
			buffer[position] = '\0';
			return buffer;
		}
		else {
			buffer[position] = c;
		}
		position++;

		//��������ʼ�ռ��С�����ٷ������ռ�
		if (position >= bufsize)
		{
			bufsize += LSH_RL_BUFSIZE;
			buffer = (char*)realloc(buffer, bufsize);
			if (!buffer)
			{
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}
	}
}

//������lsh_read_line��ʹ��getline()
string lsh_read_line2()
{
	string line = "";
	getline(std::cin, line);
	if (feof(stdin))
		exit(EXIT_SUCCESS);
	else
	{
		perror("readline");
		exit(EXIT_FAILURE);
	}
	return line;
}

#define LSH_TOK_BUFSIZE 64 //��ʼ�ռ��С
#define LSH_TOK_DELIM " \t\r\n\a"
char** lsh_split_line(char* line)
{
	int bufsize = LSH_TOK_BUFSIZE, position = 0;
	char** tokens = (char**)malloc(sizeof(char*) * bufsize);
	char* token;

	if (!tokens)
	{
		fprintf(stderr, "lsh: allocation error\n");
		exit(EXIT_FAILURE);
	}

	token = strtok(line, LSH_TOK_DELIM); //strtok���ر��ֽ�ĵ�һ�����ַ�����
										//���û�пɼ������ַ������򷵻�һ����ָ��
	while (token != NULL)
	{
		tokens[position] = token;
		position++;

		if (position >= bufsize)
		{
			bufsize += LSH_TOK_BUFSIZE;
			tokens = (char**)realloc(tokens, bufsize * sizeof(char *));
			if (!tokens)
			{
				fprintf(stderr, "lsh: allocation error\n");
				exit(EXIT_FAILURE);
			}
		}

		token = strtok(NULL, LSH_TOK_DELIM);
	}
	tokens[position] = NULL;
	return tokens;
}

int lsh_launch(char** args)
{
	pid_t pid, wpid;
	int status;

	pid = fork(); //��������
	//�� fork() ����ʱ��ʵ�������������������еĽ��̡��ӽ��̻�����һ�� if ��֧��pid == 0��
	if (pid == 0) //�ӽ���
	{
		if (execvp(args[0], args) = -1) 
		{
			//execvp��execϵͳ���õı���֮һ��execvp����һ����������һ���ַ������������飬
			//��p�� ��ʾ���ǲ���Ҫ�ṩ������ļ�·����ֻ��Ҫ�ṩ�ļ������ò���ϵͳ���������ļ���·��
			//execvp������������Ҫ���еĳ��������Ǹ�����������в�����
			//����������ʱ�����в�����argv[]�����������һ����������ΪNULL��
			perror("lsh");
		}
		exit(EXIT_FAILURE);
	}
	else if (pid < 0) //��� fork() �Ƿ����
	{
		perror("lsh"); //�������׼�豸(stderr)
	}
	else //������
	{
		do { //�ӽ���ִ������Ľ��̣���������Ҫ�ȴ��������н���
			wpid = waitpid(pid, &status, WUNTRACED); //waitpid()����ʱֹͣĿǰ���̵�ִ�У�
													//ֱ�����ź��������ӽ��̽���
		} while (!WIFEXITED(status) && !WIFSIGNALED(status)); 
		//WIFEXITED(status)����ӽ�������������Ϊ��0ֵ��
		//WIFSIGNALED(status)����ӽ�������Ϊ�źŶ�������˺�ֵΪ��
		//��ͬ��!(WIFEXITED(status)||WIFSIGNALED(status))
	}

	return 1;
}

//Shell���ú���������
int lsh_cd(char** args);
int lsh_help(char** args);
int lsh_exit(char** args);

//���������б��Լ����Ƕ�Ӧ�ĺ�����
char* builtin_str[] =
{
	"cd",
	"help",
	"exit"
};
int (*builtin_func[]) (char**) =
{
	//�������顣��������Ϊ�ˣ���δ�����Լ򵥵�ͨ���޸���Щ����������������(��չ)
	//�������޸Ĵ�����ĳ��һ���Ӵ�ġ�switch�����
	&lsh_cd,
	&lsh_help,
	&lsh_exit
};

int lsh_num_builtins() //���ú����ĸ���
{
	return sizeof(builtin_str) / sizeof(char*);
}

int lsh_cd(char** args)
{
	if (args[1] == NULL)
		fprintf(stderr, "lsh: expected argument to \"cd\"\n");
	else
	{
		if (chdir(args[1]) != 0) //chdir��C�����е�һ��ϵͳ���ú�����ͬcd�������ڸı䵱ǰ����Ŀ¼
			perror("lsh");
	}
	return 1;
}

int lsh_help(char** args)
{
	int i;
	printf("LSH\n");
	printf("Type program names and arguments, and hit enter.\n");
	printf("The following are built in:\n");

	for (i = 0; i < lsh_num_builtins(); ++i)
		printf("  %s\n", builtin_str[i]);
	
	printf("Use the man command for information on other programs.\n");
	return 1;
}

int lsh_exit(char** args)
{
	return 0; //�˳���������0������������ѭ���˳����ź�
}

int lsh_execute(char** args)
{
	int i;

	if (args[0] == NULL) //������
		return 1;

	for (i = 0; i < lsh_num_builtins(); ++i) //����Ƿ�Ҫִ�����ú���
	{
		if (strcmp(args[0], builtin_str[i]) == 0)
			return (*builtin_func[i])(args);
	}

	return lsh_launch(args);
}