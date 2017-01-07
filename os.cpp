/**
* 参与编写人员
* @ 矢小北
* 
* 任务：建立基于内存的文件系统
* 目的：深入了解文件系统结构
* 要求：首先分配一定容量的内存，建立虚拟磁盘；
*	在该磁盘上建立相应的文件文件系统；
*	为该文件系统设计相应的数据结构来管理目录，虚拟磁盘的空闲空间，已分配空间等；
*	提供文件的创建、删除、移位、改名等功能；
*	提供良好的界面，可以显示磁盘文件的状态和空间的使用情况；
*	提供虚拟磁盘转储功能，可将信息存入磁盘，还可从磁盘读入内存。
* 
*  完成于2016-12-23
*/


#include<iostream>
#include<iomanip>
#include<string>
#include<conio.h>
using namespace std;
#define NUM 256	//总节点数
#define BNUM 1000	//磁盘块数
#define BSIZE 512 //块大小
#define FSIZE 2048	//超级块大小
#define SSIZE 1046528 //存储空间大小
#define NAMESIZE 16	//文件名长度
#define DIRSIZE 16	//目录大小
#define DIRNUM 32	//目录个数
#define FREEBYTE 1021888 //空间总大小
#define DIRMODE 0 //目录类型
#define FILEMODE 1	//文件类型

// 返回状态
enum STATUS
{
	SUCCESS, ERR_PATH_FAIL, ERR_FILE_EXIST, ERR_FILE_NOT_EXIST, ERR_FILE_FAIL
};
// 超级块
struct superblock
{
	unsigned int s_size;	//总大小
	unsigned int s_itsize;	//inode表大小
	unsigned int s_freeinodesize;	//空闲i节点的数量
	unsigned int s_nextfreeinode;	//下一个空闲i节点
	unsigned int s_freeinode[NUM];	//空闲i节点数组 0/1
	unsigned int s_freeblocksize;	//空闲块的数量          
	unsigned int s_nextfreeblock;	//下一个空闲块
	unsigned char s_freeblock[BNUM];	//空闲块数组0/1  
};

//一次间接寻址
struct atime
{
	int fi_addr[128];
};
//二次间接寻址
struct mtime
{
	struct atime* atime_addr[128];
};

// 文件节点
struct finode
{
	int fi_mode;	//类型：文件/目录
	int fi_nlink;		//链接数，当链接数为0，意味着被删除
	int dir_no;		//目录号
	long int fi_size;	//文件大小
	long int fi_addr[10];	//文件块一级指针，并未实现多级指针
	struct atime  *fi_atime_unused;	//二级寻址
	struct mtime  *fi_mtime_unused;	//多级寻址
};

//目录项结构
struct direct
{
	char d_name[NAMESIZE];        //文件或者目录的名字
	unsigned short d_ino;        //文件或者目录的i节点号
};
//目录结构
struct dire
{
	struct direct direct[DIRSIZE];    //包含的目录项数组
	unsigned short size;        //包含的目录项大小
};

// 磁盘
struct storage
{
	struct superblock root;
	struct finode fnode[NUM];
	struct dire dir[DIRNUM];
	char free[FREEBYTE];
};

//全局磁盘变量
struct storage *root = new storage;
// 全局路径
char PATH[NAMESIZE*DIRNUM] = "";

// 根据路径名获取finode节点号
int getnode(char *path)
{
	if (path[0] != '/')
	{
		return -1;	//路径非法
	}
	else
	{
		struct dire cdir = root->dir[0];
		int ino = 0;
		char tpath[NAMESIZE*DIRNUM] = "";
		strcpy(tpath, path);
		char *fpath = strtok(tpath, "/");
		//cout << fpath;
		int match = 0;
		while (fpath != NULL)
		{
			match = 0;
			//cout << fpath;
			for (int i = 0; i < DIRNUM; i++)
			{
				if (!strncmp(fpath, cdir.direct[i].d_name, strlen(fpath)))
				{

					//cout << cdir.direct[i].d_name;
					if (root->fnode[cdir.direct[i].d_ino].fi_mode == DIRMODE)
					{
						ino = cdir.direct[i].d_ino;
						cdir = root->dir[root->fnode[cdir.direct[i].d_ino].dir_no];

						match = 1;
						break;
					}
					else
					{
						return -1;	//是文件，而非文件名
					}
				}

			}
			fpath = strtok(NULL, "/");
		}

		return ino;	//为根节点“/”
	}
	return -1;	//错误
}
// 在指定目录下创建文件
STATUS touch(char *path, char* fname)
{
	int ino = getnode(path);
	if (ino == -1)
	{
		return ERR_PATH_FAIL;
	}
	int n_ino;
	for (int i = 0; i < NUM; i++)
		if (root->fnode[i].fi_nlink != 1)
		{
			n_ino = i;
			root->fnode[i].fi_mode = FILEMODE;
			root->fnode[i].fi_size = 0;
			root->fnode[i].fi_addr[0] = NULL;
			root->fnode[i].fi_nlink = 1;
			break;
		}
	for (int i = 0; i < DIRSIZE; i++)
	{
		if (strlen(root->dir[root->fnode[ino].dir_no].direct[i].d_name) == 0)
		{
			root->dir[root->fnode[ino].dir_no].direct[i].d_ino = n_ino;
			strcpy(root->dir[root->fnode[ino].dir_no].direct[i].d_name, fname);
			break;
		}
	}
	return SUCCESS;
}

// 创建目录
STATUS mkdir(char *path, char* pname)
{
	int ino = getnode(path);
	if (ino == -1)
	{
		return ERR_PATH_FAIL;
	}
	int n_ino;
	int d_ino;
	// 申请新目录
	for (int i = 0; i < DIRSIZE; i++)
	{
		if (root->dir[i].size == 0)
		{
			root->dir[i].size = 1;
			d_ino = i;
			break;
			//root->dir[i].direct[0].d_ino = n_ino;
		}
	}
	//申请请finode节点
	for (int i = 0; i < NUM; i++)
		if (root->fnode[i].fi_nlink != 1)
		{
			n_ino = i;
			root->fnode[i].fi_mode = DIRMODE;
			root->fnode[i].fi_size = 0;
			root->fnode[i].dir_no = d_ino;
			root->fnode[i].fi_addr[0] = NULL;
			root->fnode[i].fi_nlink = 1;
			break;
		}
	// 在父亲节点建立指针
	for (int i = 0; i < DIRSIZE; i++)
	{
		if (strlen(root->dir[root->fnode[ino].dir_no].direct[i].d_name) == 0)
		{
			root->dir[root->fnode[ino].dir_no].direct[i].d_ino = n_ino;
			root->dir[root->fnode[ino].dir_no].size++;
			strcpy(root->dir[root->fnode[ino].dir_no].direct[i].d_name, pname);
			break;
		}
	}
	return SUCCESS;
}
// 进入目录回退上一级目录
STATUS cd(char *topath)
{
	char path[NAMESIZE*DIRNUM] = "";
	//回退上一级目录
	if (!strcmp(topath, ".."))
	{
		//
		int len;
		//cout << strlen(PATH);
		for (int i = strlen(PATH); i >= 0; i--)
		{
			if (PATH[i] == '/')
			{
				len = i;
				break;
			}
			//cout << len;
		}

		strncpy(path, PATH, len);
		strcpy(PATH, path);
	}
	// 进入目录
	else
	{
		strcpy(path, PATH);
		strcat(path, "/");
		strcat(path, topath);
		if (getnode(path) == -1 || getnode(path) == 0)
			cout << "目录输入错误，进入失败" << endl;
		else
		{
			strcpy(PATH, path);
		}

		//cout << path;

	}
	//cout << p;
	return SUCCESS;
}

// 列出目录
STATUS ls(char *path)
{
	int ino = getnode(path);
	cout << setw(10) << "NAME" << setw(5) << "type" << setw(6) << "size" << endl;
	for (int i = 0; i < DIRSIZE; i++)
	{
		if (strlen(root->dir[root->fnode[ino].dir_no].direct[i].d_name) != 0)
		{
			cout << setw(10) << root->dir[root->fnode[ino].dir_no].direct[i].d_name;
			if (root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_mode == DIRMODE)
			{
				cout << setw(5) << "DIR" << setw(6) << "-";

			}
			else
			{
				cout << setw(5) << "FILE" << setw(6) << root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_size;
			}
			cout << endl;
		}
	}
	return SUCCESS;
}

// 文件改名
STATUS rename(char *path, char *cname, char *nname)
{
	int ino = getnode(path);
	for (int i = 0; i < DIRSIZE; i++)
	{
		if (strcmp(root->dir[root->fnode[ino].dir_no].direct[i].d_name, cname) == 0)
		{
			for (int j = 0; j < DIRSIZE; j++)
			{
				if (strcmp(root->dir[root->fnode[ino].dir_no].direct[j].d_name, nname) == 0)
				{
					cout << "文件名重复" << endl;
					return ERR_FILE_EXIST;
				}
			}
			strcpy(root->dir[root->fnode[ino].dir_no].direct[i].d_name, nname);
		}
	}
	return SUCCESS;
}
// 文件移位
STATUS mv(char *path, char *file, char *npath)
{

	int ino = getnode(path);
	//从目录中删除节点
	int n_ino;
	for (int i = 0; i < DIRSIZE; i++)
	{
		if (strcmp(root->dir[root->fnode[ino].dir_no].direct[i].d_name, file) == 0)
		{
			strcpy(root->dir[root->fnode[ino].dir_no].direct[i].d_name, "");
			n_ino = root->dir[root->fnode[ino].dir_no].direct[i].d_ino;
			root->dir[root->fnode[ino].dir_no].size--;
		}
	}
	ino = getnode(npath);
	// 建立新节点表
	for (int i = 0; i < DIRSIZE; i++)
	{
		if (strlen(root->dir[root->fnode[ino].dir_no].direct[i].d_name) == 0)
		{
			root->dir[root->fnode[ino].dir_no].direct[i].d_ino = n_ino;
			strcpy(root->dir[root->fnode[ino].dir_no].direct[i].d_name, file);
			break;
		}
	}

	return SUCCESS;
}

// 删除文件
int rm(char *path, char *file)
{
	int ino = getnode(path);

	int n_ino;
	//从目录中移除节点
	for (int i = 0; i < DIRSIZE; i++)
	{
		if (strcmp(root->dir[root->fnode[ino].dir_no].direct[i].d_name, file) == 0)
		{
			strcpy(root->dir[root->fnode[ino].dir_no].direct[i].d_name, "");
			n_ino = root->dir[root->fnode[ino].dir_no].direct[i].d_ino;
			root->dir[root->fnode[ino].dir_no].size--;
		}
		else
		{
			cout << "文件不存在" << endl;
			return ERR_FILE_NOT_EXIST;
		}
	}
	for (int i = 0; i < 1 + (root->fnode[n_ino].fi_size / BSIZE); i++)
	{
		root->root.s_freeblock[root->fnode[n_ino].fi_addr[i]] = 0;
		root->root.s_freeblocksize++;
	}
	root->fnode[n_ino].fi_nlink = 0;
	return SUCCESS;
}

// 查询空闲空间
STATUS free()
{
	cout << "[";
	int l = (int)40.0*(1.0*root->root.s_freeblocksize / BNUM);
	//cout << l;

	for (int i = 0; i < 40 - l; i++)
	{
		cout << "=";
	}
	for (int i = 0; i < l; i++)
	{
		cout << " ";
	}
	cout << "] ";
	cout << (int)(100.0*root->root.s_freeblocksize / BNUM) << "% free" << endl;
	return SUCCESS;
}

//读文件
STATUS cat(char *path, char *file)
{
	int ino = getnode(path);
	for (int i = 0; i < DIRSIZE; i++)
	{
		if (strcmp(root->dir[root->fnode[ino].dir_no].direct[i].d_name, file) == 0)
		{
			if (root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_mode == DIRMODE)
			{
				cout << "这是一个目录" << endl;
			}
			else
			{
				// 确定有多少块
				for (int j = 0; j < 1 + root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_size / BSIZE; j++)
				{
					//cout << (BSIZE *root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_addr[j]) << endl;
					//cout << ((root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_addr[j] * BSIZE) + (
					//	j>(root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_size / BSIZE) ? BSIZE :
					//	(root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_size%BSIZE)));
					//cout << (root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_size / BSIZE) << endl;;
					for (int k = (BSIZE *root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_addr[j]);
						k < ((root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_addr[j] * BSIZE) +
						(j > (root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_size / BSIZE) ? BSIZE :
							(root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_size%BSIZE))); k++)
					{
						//cout << k;
						cout << root->free[k];
					}
				}
			}
			break;
		}
	}
	cout << endl;
	return SUCCESS;
}
// 写文件
STATUS vi(char *path, char *file, char *cont)
{
	int ino = getnode(path);
	for (int i = 0; i < DIRSIZE; i++)
	{
		if (strcmp(root->dir[root->fnode[ino].dir_no].direct[i].d_name, file) == 0)
		{
			if (root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_mode == DIRMODE)
			{
				cout << "这是一个目录" << endl;
			}
			else
			{

				root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_size = strlen(cont);
				// 确定有多少块
				for (int j = 0; j < 1 + strlen(cont) / BSIZE; j++)
				{
					// 寻找空闲块
					for (int k = 0; k < BNUM; k++)
					{

						if (root->root.s_freeblock[k] != 1)
						{
							// 写入空间块
							int l, m;
							root->root.s_freeblock[k] = 1;
							root->root.s_freeblocksize--;
							root->fnode[root->dir[root->fnode[ino].dir_no].direct[i].d_ino].fi_addr[j] = k;
							for (l = k*BSIZE, m = j*BSIZE; l < k*BSIZE + BSIZE; m++, l++)
							{
								root->free[l] = cont[m];

							}
							break;
						}
					}
				}
			}
			break;
		}

	}
	return SUCCESS;
}

// 写出文件
STATUS writeout()
{
	FILE *fp;
	if ((fp = fopen("filesystem", "w")) == NULL)
	{
		cout << "写出文件失败！" << endl;
		return ERR_FILE_FAIL;
	}

	if (fwrite(root, sizeof(struct storage), 1, fp) != 1)
	{
		cout << "文件写出失败" << endl;
	}

	fclose(fp);
	return SUCCESS;
}
//读取文件
STATUS readin()
{

	FILE *fp;
	if ((fp = fopen("filesystem", "r")) == NULL)
	{
		cout << "读入文件失败" << endl;
		return ERR_FILE_FAIL;
	}
	if (fread(root, sizeof(struct storage), 1, fp))
	{
		fclose(fp);
		return SUCCESS;
	}
	return SUCCESS;
}
// 系统初始化
void init()
{

	memset(root->fnode, '\0', FREEBYTE);
	root->dir[0].direct[0].d_ino = 0;
	root->dir[0].size = 1;
	strcpy(root->dir[0].direct[0].d_name, "C"); //设置根目录名
	root->fnode[0].fi_mode = DIRMODE;
	root->fnode[0].fi_nlink = 1;
	root->fnode[0].dir_no = 1;
	root->dir[1].size = 1;
	root->root.s_freeblocksize = BNUM;
	strcpy(PATH, "/C");
}
int  menu()
{
	cout << "*****************************************************************************" << endl;
	cout << "*                             文件系统操作手册                               *" << endl;
	cout << "*                                                                            *" << endl;
	cout << "*                  1、 mkdir <dir>  --创建目录                               *" << endl;
	cout << "*                  2、 touch <file>  --创建文件                              *" << endl;
	cout << "*                  3、 cat <file>  --读取文件                                *" << endl;
	cout << "*                  4、 vi <file>   --编辑文件                                *" << endl;
	cout << "*                  5、 rm <file>  --删除目录/文件                            *" << endl;
	cout << "*                  6、 rename <src> <dest>  --重命名                         *" << endl;
	cout << "*                  7、 mv <file> <dir>  --移动文件                           *" << endl;
	cout << "*                  8、 cd <dir>  --打开目录                                  *" << endl;
	cout << "*                  9、 cd ..  --返回上一级目录                               *" << endl;
	cout << "*                  10、pwd  --查看当前路径                                   *" << endl;
	cout << "*                  11、ls  --列现当前目录                                    *" << endl;
	cout << "*                  12、free  --显示磁盘可用空间                              *" << endl;
	cout << "*                  13、writeout  --写入到磁盘                                *" << endl;
	cout << "*                  14、readin  --从磁盘读取文件系统                          *" << endl;
	cout << "*                  15、format  --格式化磁盘                                  *" << endl;
	cout << "*                  16、help  --显示帮助信息                                  *" << endl;
	cout << "*                  17、exit  --退出文件系统                                  *" << endl;
	cout << "******************************************************************************" << endl;
	return 0;
}
int main()
{

	menu();
	string s;
	char arg1[NAMESIZE] = "";
	char arg2[NAMESIZE] = "";
	char content[BSIZE*BNUM] = "";
	init();
	while (1)
	{
		cin >> s;
		if (s == "mkdir")
		{
			//cout << "mkdir";
			cin >> arg1;
			mkdir(PATH, arg1);
			ls(PATH);

		}
		else if (s == "format")
		{
			memset(root, '\0', sizeof(struct storage));
			init();
		}
		else if (s == "touch")
		{
			cin >> arg1;
			touch(PATH, arg1);
			//ls(PATH);
		}
		else if (s == "rm")
		{
			cin >> arg1;
			rm(PATH, arg1);
			//ls(PATH);
		}
		else if (s == "mv")
		{
			cin >> arg1 >> arg2;
			mv(PATH, arg1, arg2);
			//ls(PATH);
		}
		else if (s == "rename")
		{
			cin >> arg1 >> arg2;
			rename(PATH, arg1, arg2);
			//ls(PATH);
		}
		else if (s == "cat")
		{
			cin >> arg1;
			cat(PATH, arg1);
			//ls(PATH);
		}
		else if (s == "vi")
		{
			cin >> arg1;
			cout << endl << "请输入要写入文件的内容：" << endl;
			cin >> content;
			vi(PATH, arg1, content);
			//ls(PATH);
		}
		else if (s == "cd")
		{
			cin >> arg1;
			cd(arg1);
			//ls(PATH);
		}
		else if (s == "pwd")
		{
			cout << PATH << endl;
		}
		else if (s == "ls")
		{
			ls(PATH);
		}
		else if (s == "writeout")
		{
			writeout();
			cout << "已写出到 filesystem.dat" << endl;
			// 写出到 filesystem.dat
		}
		else if (s == "readin")
		{
			readin();
			cout << "已从 filesystem.dat 中读入文件系统" << endl;
		}
		else if (s == "exit")
		{
			break;
		}
		else if (s == "free")
		{
			free();
		}
		else
		{
			cout << "命令错误，请重新输入！" << endl;
		}
	}
	return 0;
}
