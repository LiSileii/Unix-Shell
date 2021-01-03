#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;

int main()
{
	//1.初始化：加载配置文件

	//2.解释执行：运行命令（循环）
	lsh_loop();

	//3.终止：执行关闭命令

	return EXIT_SUCCESS;
}

void lsh_loop()
{
	char* line;
	char** args;
	int status;

	do { //Do-while 循环在检查状态变量时会更方便，因为它会在检查变量的值之前先执行一次
		printf("> ");
		//1.读取命令
		line = lsh_read_line();
		//2.分析
		args = lsh_split_line(line);
		//3.执行
		status = lsh_execute(args); //根据执行结果判断是否退出循环

		//释放之前为line和args申请的内存空间，这样的话就不用将line和args定义在循环内
		free(line);
		free(args);
	} while (status);
}

#define LSH_RL_BUFSIZE 1024 //初始空间大小
//constexpr auto LSH_RL_BUFSIZE = 1024; //也可用constexpr
char* lsh_read_line()
{
	int bufsize = LSH_RL_BUFSIZE;
	int position = 0;
	char* buffer = (char *)malloc(sizeof(char) * bufsize); //分配初始内存空间
	int c; //读入的字符保存为int类型而不是char类型
		//因为EOF是一个整型值而不是字符型值。如果想将它的值作为判断条件，就需要使用int类型

	if (!buffer)
	{
		fprintf(stderr, "lsh: allocation error\n"); //fprintf格式化输出到一个流文件中
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		c = getchar(); //读入一个字符
		if (c == EOF || c == '\n') //读到输入末尾
		{
			buffer[position] = '\0';
			return buffer;
		}
		else {
			buffer[position] = c;
		}
		position++;

		//若超出初始空间大小，则再分配更多空间
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

//更简版的lsh_read_line，使用getline()
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

#define LSH_TOK_BUFSIZE 64 //初始空间大小
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

	token = strtok(line, LSH_TOK_DELIM); //strtok返回被分解的第一个子字符串，
										//如果没有可检索的字符串，则返回一个空指针
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

	pid = fork(); //拷贝进程
	//当 fork() 返回时，实际上有了两个并发运行的进程。子进程会进入第一个 if 分支（pid == 0）
	if (pid == 0) //子进程
	{
		if (execvp(args[0], args) = -1) 
		{
			//execvp是exec系统调用的变体之一，execvp接受一个程序名和一个字符串参数的数组，
			//‘p’ 表示我们不需要提供程序的文件路径，只需要提供文件名，让操作系统搜索程序文件的路径
			//execvp有两个参数：要运行的程序名和那个程序的命令行参数。
			//当程序运行时命令行参数以argv[]传给程序。最后一个参数必须为NULL。
			perror("lsh");
		}
		exit(EXIT_FAILURE);
	}
	else if (pid < 0) //检查 fork() 是否出错
	{
		perror("lsh"); //输出到标准设备(stderr)
	}
	else //父进程
	{
		do { //子进程执行命令的进程，父进程需要等待命令运行结束
			wpid = waitpid(pid, &status, WUNTRACED); //waitpid()会暂时停止目前进程的执行，
													//直到有信号来到或子进程结束
		} while (!WIFEXITED(status) && !WIFSIGNALED(status)); 
		//WIFEXITED(status)如果子进程正常结束则为非0值。
		//WIFSIGNALED(status)如果子进程是因为信号而结束则此宏值为真
		//等同于!(WIFEXITED(status)||WIFSIGNALED(status))
	}

	return 1;
}

//Shell内置函数的声明
int lsh_cd(char** args);
int lsh_help(char** args);
int lsh_exit(char** args);

//内置命令列表，以及它们对应的函数。
char* builtin_str[] =
{
	"cd",
	"help",
	"exit"
};
int (*builtin_func[]) (char**) =
{
	//函数数组。这样做是为了，在未来可以简单地通过修改这些数组来添加内置命令，(扩展)
	//而不是修改代码中某处一个庞大的“switch”语句
	&lsh_cd,
	&lsh_help,
	&lsh_exit
};

int lsh_num_builtins() //内置函数的个数
{
	return sizeof(builtin_str) / sizeof(char*);
}

int lsh_cd(char** args)
{
	if (args[1] == NULL)
		fprintf(stderr, "lsh: expected argument to \"cd\"\n");
	else
	{
		if (chdir(args[1]) != 0) //chdir是C语言中的一个系统调用函数（同cd），用于改变当前工作目录
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
	return 0; //退出函数返回0，这是让命令循环退出的信号
}

int lsh_execute(char** args)
{
	int i;

	if (args[0] == NULL) //空命令
		return 1;

	for (i = 0; i < lsh_num_builtins(); ++i) //检查是否要执行内置函数
	{
		if (strcmp(args[0], builtin_str[i]) == 0)
			return (*builtin_func[i])(args);
	}

	return lsh_launch(args);
}