/*文件名不支持中文及符号。*/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
using namespace std;
typedef unsigned char u8;   //1字节
typedef unsigned short u16; //2字节
typedef unsigned int u32;   //4字节
int BytsPerSec;				//每扇区字节数
int SecPerClus;				//每簇扇区数
int RsvdSecCnt;				//Boot记录占用的扇区数
int NumFATs;				//FAT表个数
int RootEntCnt;				//根目录最大文件数
int FATSz;					//FAT扇区数
//FAT1的偏移字节
int fatBase;
int fileRootBase;
int dataBase;
int BytsPerClus;
#pragma pack(1) /*指定按1字节对齐*/

//偏移11个字节
struct BPB
{
	u16 BPB_BytsPerSec; //每扇区字节数
	u8 BPB_SecPerClus;  //每簇扇区数
	u16 BPB_RsvdSecCnt; //Boot记录占用的扇区数
	u8 BPB_NumFATs;		//FAT表个数
	u16 BPB_RootEntCnt; //根目录最大文件数
	u16 BPB_TotSec16;
	u8 BPB_Media;
	u16 BPB_FATSz16; //FAT扇区数
	u16 BPB_SecPerTrk;
	u16 BPB_NumHeads;
	u32 BPB_HiddSec;
	u32 BPB_TotSec32; //如果BPB_FATSz16为0，该值为FAT扇区数
};
//BPB至此结束，长度25字节
//根目录条目
struct RootEntry
{
	char DIR_Name[11];
	u8 DIR_Attr; //文件属性
	char reserved[10];
	u16 DIR_WrtTime;
	u16 DIR_WrtDate;
	u16 DIR_FstClus; //开始簇号
	u32 DIR_FileSize;
};
//根目录条目结束，32字节

#pragma pack() /*取消指定对齐，恢复缺省对齐*/

typedef struct RootEntry *fptr;
struct fnode
{
	char rname[40] = {0};
	char fname[12] = {0};
	RootEntry fentry;
	void append(const char *ch) { strcat(fname, ch); }
	void rappend(const char *ch) { strcat(rname, ch); }
};
struct lsans
{
	bool flag;
	char fname[12];
	void append(char *ch) { strcat(fname, ch); }
	void ini()
	{
		cntf = cntr = 0;
		memset(fname, 0, sizeof(fname));
	}
	int cntr, cntf, fsize;
};
vector<lsans> vcans;
vector<fnode> ftree;
vector<fnode> froot;
char fname_tmp[11];
char Qfroot[40];
void ini(FILE *);
char split[2] = "/";
char symb[3] = "/:";
int Rcnt1, Rcnt2;
void parsecmd(FILE *, char *cmd);
int checkFile(char *, int);
void getFname(const char *);
void getRname(const char *);
int getqname(const char *, int st);
void getRootFiles(FILE *fat12, struct RootEntry *rootEntry_ptr); //打印文件名，这个函数在打印目录时会调用下面的printChildren
void printChildren(FILE *fat12, fnode &);						 //打印目录及目录下子文件名
int getFATValue(FILE *fat12, int num);							 //读取num号FAT项所在的两个字节，并从这两个连续字节中取出FAT项的值，
pair<int, int> getcnt(RootEntry re, FILE *fat12);
void dfs(FILE *fat12, fnode cur, char *rname);
pair<int, int> solveRootFile(fnode cur, FILE *fat12);
extern "C" void my_print(char *, int, int);
void ls_l(FILE *fat12);
void ls(FILE *fat12);
int checkLetter(char x);
char transletter(char x);
void lsr(FILE *fat12);
void lslr(FILE *fat12);
char tdata[100000];
char warn1[43] = "The file is not found, please retype it.\n";
char warn2[44] = "Please enter the correct command.\n";
char warn3[44] = "Please enter a filename in uppercase.\n";
char warn4[50] = "Missing space between filename and command.\n";
char blankSpace[2] = " ";
char newLine[2] = "\n";
char dot1[10] = ".  ..  ";
char dot2[10] = ".\n..\n";
char colon[2] = ":";
char inp[2]=">";
void getTextData(FILE *fat12, RootEntry re)
{
	int curClus = re.DIR_FstClus;
	int startByte;
	while (curClus < 0xFF8)
	{
		if (curClus == 0xFF7)
		{
			char mp[30] = "bad cluster,read failed\n";
			my_print(mp, strlen(mp), 1);
			break;
		}
		startByte = dataBase + (curClus - 2) * BytsPerClus;
		fseek(fat12, startByte, SEEK_SET);
		fread(tdata, 1, BytsPerClus, fat12);
		cout<<tdata;
		curClus = getFATValue(fat12, curClus); //获取fat项的内容
	}
}
void getTextEntry(FILE *fat12, RootEntry re, string tname)
{
	int curClus = re.DIR_FstClus;
	int startByte;
	while (curClus < 0xFF8)
	{
		if (curClus == 0xFF7)
		{
			char mp[30] = "bad cluster,read failed\n";
			my_print(mp, strlen(mp), 1);
			break;
		}
		startByte = dataBase + (curClus - 2) * BytsPerClus;
		for (int loop = 0; loop < BytsPerClus; loop += 32)
		{
			RootEntry roottmp;
			fptr rootptr = &roottmp;
			fseek(fat12, startByte + loop, SEEK_SET);
			fread(rootptr, 1, 32, fat12);
			if ((rootptr->DIR_Name[0] == '\0') || (checkFile(rootptr->DIR_Name, 0) == 0) || (rootptr->DIR_Attr & 0x10) != 0)
				continue;
			getFname(rootptr->DIR_Name);
			string tmp = fname_tmp;
			if (tmp == tname)
			{
				getTextData(fat12, *rootptr);
				break;
			}
		}
		curClus = getFATValue(fat12, curClus); //获取fat项的内容
	}
}
// cat DATA.TXT
void solveCat(FILE *fat12)
{
	string str = Qfroot, sroot, stext;
	bool flag = 0;
	int pos = str.rfind('/');
	if (pos < 0)
	{
		stext = str;
		for (auto ele : froot)
		{
			if (strcmp(ele.fname, Qfroot) == 0)
			{
				getTextData(fat12, ele.fentry);
				flag = 1;
				break;
			}
		}
	}
	sroot = str.substr(0, pos);
	stext = str.substr(pos + 1);
	for (auto ele : ftree)
	{
		string x = ele.rname;
		x += ele.fname;
		if (x == sroot)
		{
			getTextEntry(fat12, ele.fentry, stext);
			flag = 1;
			break;
		}
	}
	if (flag == 0)
	{
		my_print(warn1, strlen(warn1), 1);
	}
}
//TODO: 查找文件报错

int main()
{
	FILE *fat12;
	fat12 = fopen("/home/sweet/code/a.img", "rb"); //打开FAT12的映像文件
	ini(fat12);
	struct RootEntry rootEntry;
	fptr rootEntry_ptr = &rootEntry;
	getRootFiles(fat12, rootEntry_ptr);
	for (auto ele : froot)
	{
		if ((ele.fentry.DIR_Attr & 0x10) == 0)
			continue;
		char tmp[50];
		memset(tmp, 0, sizeof(tmp));
		strcat(tmp, ele.rname);
		strcat(tmp, ele.fname);
		ftree.push_back(ele);
		dfs(fat12, ele, tmp);
	}
	char cmd[50];
my_print(inp,strlen(inp),0);
	while (cin.getline(cmd, 50))
	{
		if (cmd[0] == 'q' || cmd[0] == 'Q')
			break;
		parsecmd(fat12, cmd);
		memset(cmd, 0, sizeof(cmd));
	my_print(inp,strlen(inp),0);
		// break;
	}
	fclose(fat12);
	//system("pause");
}

void getRootFiles(FILE *fat12, fptr rootEntry_ptr)
{
	int frbase = fileRootBase;
	for (int i = 0; i < RootEntCnt; i++)
	{
		fseek(fat12, frbase, SEEK_SET);
		fread(rootEntry_ptr, 1, 32, fat12);
		frbase += 32;
		if ((rootEntry_ptr->DIR_Name[0] == '\0') || (checkFile(rootEntry_ptr->DIR_Name, 0) == 0)) //过滤非法条目
			continue;
		fnode f;
		if ((rootEntry_ptr->DIR_Attr & 0x10) == 0) //此条目是文件
		{
			getFname(rootEntry_ptr->DIR_Name);
			Rcnt2++;
		}
		else //目录   则放进队列
		{
			getRname(rootEntry_ptr->DIR_Name);
			f.rappend(split);
			Rcnt1++;
		}
		f.append(fname_tmp);
		f.fentry = *rootEntry_ptr;
		froot.push_back(f);
	}
}
int getFATValue(FILE *fat12, int num)
{
	u16 bytes;
	u16 *bytes_ptr = &bytes;
	fseek(fat12, fatBase + num * 3 / 2, SEEK_SET);
	fread(bytes_ptr, 1, 2, fat12);								 //先读出FAT项所在的两个字节 16位
	return (num & 1) ? (bytes >> 4) : (bytes & ((1 << 12) - 1)); //结合存储的小尾顺序和FAT项结构可以得到。偶去掉高4位,奇去掉低4位
}
int checkFile(char *fname_tmp, int pos)
{
	for (int j = pos; j < pos + 11; j++)
	{
		if (!(((fname_tmp[j] >= 48) && (fname_tmp[j] <= 57)) ||
			  ((fname_tmp[j] >= 65) && (fname_tmp[j] <= 90)) ||
			  ((fname_tmp[j] >= 97) && (fname_tmp[j] <= 122)) ||
			  (fname_tmp[j] == ' ')))
			return 0;
	}
	return 1;
}
void getRname(const char *dirname)
{
	int tmplen = 0;
	for (int k = 0; k < 11 && dirname[k] != ' '; k++)
		fname_tmp[tmplen++] = transletter(dirname[k]);
	fname_tmp[tmplen] = '\0';
}
void getFname(const char *dirname)
{
	int tmplen = 0;
	for (int k = 0; k < 11; k++)
	{
		if (dirname[k] != ' ')
			fname_tmp[tmplen++] = transletter(dirname[k]);
		else
		{
			fname_tmp[tmplen++] = '.';
			while ((dirname[k] == ' ') && k < 11) // 过滤空格
				k++;
			k--;
		}
	}
	fname_tmp[tmplen] = '\0';
}
void ini(FILE *fat12)
{
	struct BPB bpb;
	struct BPB *bpb_ptr = &bpb;   //载入BPB
	fseek(fat12, 11, SEEK_SET);   //BPB从偏移11个字节处开始
	fread(bpb_ptr, 1, 25, fat12); //BPB长度为25字节

	BytsPerSec = bpb_ptr->BPB_BytsPerSec; //初始化各个全局变量
	SecPerClus = bpb_ptr->BPB_SecPerClus;
	RsvdSecCnt = bpb_ptr->BPB_RsvdSecCnt;
	NumFATs = bpb_ptr->BPB_NumFATs;
	RootEntCnt = bpb_ptr->BPB_RootEntCnt;
	if (bpb_ptr->BPB_FATSz16 != 0)
		FATSz = bpb_ptr->BPB_FATSz16;
	else
		FATSz = bpb_ptr->BPB_TotSec32;
	fatBase = RsvdSecCnt * BytsPerSec;
	fileRootBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec; //根目录首字节的偏移数=boot+fat1&2的总字节数
	dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
	BytsPerClus = SecPerClus * BytsPerSec; //每簇的字节数
}
void dfs(FILE *fat12, fnode cur, char *rname)
{
	int curClus = cur.fentry.DIR_FstClus;
	int startByte;
	while (curClus < 0xFF8)
	{
		if (curClus == 0xFF7)
		{
			char mp[30] = "bad cluster,read failed\n";
			my_print(mp, strlen(mp), 1);
			break;
		}
		startByte = dataBase + (curClus - 2) * BytsPerClus;
		for (int loop = 0; loop < BytsPerClus; loop += 32)
		{
			RootEntry roottmp;
			fptr rootptr = &roottmp;
			fseek(fat12, startByte + loop, SEEK_SET);
			fread(rootptr, 1, 32, fat12);
			if ((rootptr->DIR_Name[0] == '\0') || (checkFile(rootptr->DIR_Name, 0) == 0) || (rootptr->DIR_Attr & 0x10) == 0)
				continue;
			getRname(rootptr->DIR_Name);
			fnode f;
			f.append(fname_tmp);
			f.rappend(rname);
			f.rappend(split);
			f.fentry = *rootptr;
			char tmp[50];
			memset(tmp, 0, sizeof(tmp));
			strcat(tmp, f.rname);
			strcat(tmp, split);
			strcat(tmp, f.fname);
			ftree.push_back(f);
			dfs(fat12, f, tmp);
		}
		curClus = getFATValue(fat12, curClus); //获取fat项的内容
	}
}
pair<int, int> getcnt(RootEntry re, FILE *fat12)
{

	pair<int, int> ans;
	ans.first = 0, ans.second = 0;
	int curClus = re.DIR_FstClus;
	int startByte;
	while (curClus < 0xFF8)
	{
		if (curClus == 0xFF7)
		{
			char mp[30] = "bad cluster,read failed\n";
			my_print(mp, strlen(mp), 1);
			break;
		}
		startByte = dataBase + (curClus - 2) * BytsPerClus;
		for (int loop = 0; loop < BytsPerClus; loop += 32)
		{
			RootEntry roottmp;
			fptr rootptr = &roottmp;
			fseek(fat12, startByte + loop, SEEK_SET);
			fread(rootptr, 1, 32, fat12);
			if ((rootptr->DIR_Name[0] == '\0') || (checkFile(rootptr->DIR_Name, 0) == 0))
				continue;

			if ((rootptr->DIR_Attr & 0x10) == 0)
				ans.second++;
			else
				ans.first++;
		}
		curClus = getFATValue(fat12, curClus); //获取fat项的内容
	}
	return ans;
}
pair<int, int> solveRootFile(fnode cur, FILE *fat12)
{
	vcans.clear();
	int ans1 = 0, ans2 = 0;
	int curClus = cur.fentry.DIR_FstClus, startByte;
	while (curClus < 0xFF8)
	{
		if (curClus == 0xFF7)
		{
			char mp[30] = "bad cluster,read failed\n";
			my_print(mp, strlen(mp), 1);
			break;
		}
		startByte = dataBase + (curClus - 2) * BytsPerClus;
		for (int loop = 0; loop < BytsPerClus; loop += 32)
		{
			RootEntry roottmp;
			fptr rootptr = &roottmp;
			fseek(fat12, startByte + loop, SEEK_SET);
			fread(rootptr, 1, 32, fat12);
			if ((rootptr->DIR_Name[0] == '\0') || (checkFile(rootptr->DIR_Name, 0) == 0))
				continue;
			if ((rootptr->DIR_Attr & 0x10) == 0)
			{
				lsans leaf;
				leaf.ini();
				leaf.flag = 0;
				ans2++;
				getFname(rootptr->DIR_Name);
				leaf.append(fname_tmp);
				leaf.fsize = rootptr->DIR_FileSize;
				vcans.push_back(leaf);
			}
			else
			{
				lsans leaf;
				leaf.ini();
				ans1++;
				leaf.flag = 1;
				getRname(rootptr->DIR_Name);
				leaf.append(fname_tmp);
				pair<int, int> ans = getcnt(*rootptr, fat12);
				leaf.cntr = ans.first, leaf.cntf = ans.second;
				vcans.push_back(leaf);
			}
		}
		curClus = getFATValue(fat12, curClus); //获取fat项的内容
	}
	return {ans1, ans2};
}

void ls(FILE *fat12)
{
	pair<int, int> ptmp;
	int mode;
	my_print(symb, strlen(symb), 0);
	for (auto ele : froot)
	{
		mode = (ele.fentry.DIR_Attr & 0x10); //0-file
		my_print(ele.fname, strlen(ele.fname), mode);
		my_print(blankSpace, strlen(blankSpace), 0);
	}
	my_print(newLine, strlen(newLine), 0);

	for (auto ele : ftree)
	{
		pair<int, int> rans = solveRootFile(ele, fat12);
		my_print(ele.rname, strlen(ele.rname), 0);
		my_print(ele.fname, strlen(ele.fname), 0);
		my_print(symb, strlen(symb), 0);
		my_print(newLine, strlen(newLine), 0);
		my_print(dot1, strlen(dot1), 1);
		for (auto obj : vcans)
		{
			my_print(obj.fname, strlen(obj.fname), obj.flag);
			my_print(blankSpace, strlen(blankSpace), 0);
		}
		my_print(newLine, strlen(newLine), 0);
	}
}
//标点和文件名注意用红色！
void ls_l(FILE *fat12)
{
	my_print(split, strlen(split), 0);
	my_print(blankSpace, strlen(blankSpace), 0);
	
	char itmp[100];
	sprintf(itmp, "%d", Rcnt1);
	my_print(itmp, strlen(itmp), 0);
	my_print(blankSpace, strlen(blankSpace), 0);
	sprintf(itmp, "%d", Rcnt2);
	my_print(itmp, strlen(itmp), 0);
	my_print(newLine, strlen(newLine), 0);

	my_print(dot2, strlen(dot2), 1);
	for (auto ele : froot)
	{
		if ((ele.fentry.DIR_Attr & 0x10) == 0)
		{
			my_print(ele.fname, strlen(ele.fname), 0);
			my_print(blankSpace, strlen(blankSpace), 0);
			sprintf(itmp, "%d", ele.fentry.DIR_FileSize);
			my_print(itmp, strlen(itmp), 0);
		}
		else
		{
			pair<int, int> ans = getcnt(ele.fentry, fat12);
			my_print(ele.fname, strlen(ele.fname), 1);
			my_print(blankSpace, strlen(blankSpace), 0);
			sprintf(itmp, "%d", ans.first);
			my_print(itmp, strlen(itmp), 0);
			my_print(blankSpace, strlen(blankSpace), 0);
			sprintf(itmp, "%d", ans.second);
			my_print(itmp, strlen(itmp), 0);
			my_print(newLine, strlen(newLine), 0);
		}
	}
	for (auto ele : ftree)
	{
		pair<int, int> rans = solveRootFile(ele, fat12);
		my_print(ele.rname, strlen(ele.rname), 0);
		my_print(ele.fname, strlen(ele.fname), 0);
		my_print(blankSpace, strlen(split), 0);
		my_print(blankSpace, strlen(blankSpace), 0);
		sprintf(itmp, "%d", rans.first);
		my_print(itmp, strlen(itmp), 0);
		my_print(blankSpace, strlen(blankSpace), 0);
		sprintf(itmp, "%d", rans.second);
		my_print(itmp, strlen(itmp), 0);
		my_print(colon, strlen(colon), 0);
		my_print(newLine, strlen(newLine), 0);
		// << ele.rname << ele.fname << split << " " << rans.first << " " << rans.second << ":" << endl;
		my_print(dot2, strlen(dot2), 1);
		for (auto obj : vcans)
		{
			//  << obj.fname << " " << obj.cntr << " " << obj.cntf << endl;
			if (obj.flag)
			{
				my_print(obj.fname, strlen(obj.fname), 1);
				my_print(blankSpace, strlen(blankSpace), 0);
				sprintf(itmp, "%d",  obj.cntr );
				my_print(itmp, strlen(itmp), 0);
				my_print(blankSpace, strlen(blankSpace), 0);
				sprintf(itmp, "%d",  obj.cntf);
				my_print(itmp, strlen(itmp), 0);
				my_print(blankSpace, strlen(blankSpace), 0);
				my_print(newLine, strlen(newLine), 0);
			}
			else
			{
				my_print(obj.fname, strlen(obj.fname), 0);
				my_print(blankSpace, strlen(blankSpace), 0);
				sprintf(itmp, "%d", obj.fsize);
				my_print(itmp, strlen(itmp), 0);
				my_print(newLine, strlen(newLine), 0);
			}
			//  << obj.fname << " " << obj.fsize << endl;
		}
		// my_print(newLine, strlen(newLine), 0);
	}
}
int checkLetter(char c)
{
	if (c == '/')
		return 1;
	if (c <= 'z' && c >= 'a')
		return -1;
	else if (c <= 'Z' && c >= 'A')
		return 1;
	else if (c <= '9' && c >= '1')
		return 1;
	else
		return 0;
}
char transletter(char x)
{
	if ((x <= 'z') && (x >= 'a'))
	{
		x -= 32;
	}
	return x;
}
// ls -NJU -l
void parsecmd(FILE *fat12, char *cmd)
{
	int len = strlen(cmd);
	bool isls = 1;
	bool lsf1 = 0, lsf2 = 0;
	int comeFirst; //先出现的是-还是/
	//这里检测在命令cat/ls前面有没有输入非法字符
	int i = 0;
	while (cmd[i] == ' ' && i < len)
		i++;
	//如果第一个字符非法 跳过
	if (cmd[i] != 'l' && cmd[i] != 'c')
	{
		my_print(warn2, strlen(warn2), 1);
		return;
	}
	//如果是ls命令
	if (cmd[i] == 'l' && cmd[i + 1] == 's')
	{
		i += 2;
		for (int j = i; j < len; j++)
		{
			if (cmd[j] != ' ')
				isls = 0;
		}
		//如果ls后面没有字符了 那么就是ls命令
		if (isls == 1)
		{
			ls(fat12);
			return;
		}
		//过滤ls后面的空格
		if (cmd[i] == ' ')
		{
			while (cmd[i] == ' ')
				i++;
		}
		else //ls后面没有空格
		{
			my_print(warn4, strlen(warn4), 1);//lshoum 
			return;
		}
		//如果是ls -l 加文件, 取文件名
		if (cmd[i] == '-' && cmd[i + 1] == 'l')
		{
			i += 2;
			while (i < len && cmd[i] == 'l')
				i++;
			//ls -lll命令
			if (i == len)
			{
				ls_l(fat12);
				return;
			}
			//过滤-l后面的空格
			if (cmd[i] == ' ')
			{
				while (i < len && cmd[i] == ' ') //有空格过滤空格
					i++;
				if (i == len) //ls -lll
				{
					ls_l(fat12);
					return;
				}
			}
			else if (i == len)
			{
				ls_l(fat12);
				return;
			}
			else //-l后面没有空格直接跟着字符
			{
				my_print(warn4, strlen(warn4), 1);//lshoum 
				return;
			}
			if (cmd[i] == '/')
			{
				if (getqname(cmd, i))
				{
					lslr(fat12);
				}
			}	//ls /NJU
			else //-l后面没有空格
			{
					my_print(warn2, strlen(warn2), 1);//lshoum 
				return;
			}
		}
		else if (cmd[i] == '/') //如果后面直接跟着/
		{
			if (getqname(cmd, i) == 1)
			{
				lslr(fat12);
			}
			else if(getqname(cmd,i)==2)
			{
				lsr(fat12);
			}
			else
			return;
		}
		else
		{
			my_print(warn2, strlen(warn2), 1);
			return;
		}
	}
	if (cmd[i] == 'c' && cmd[i + 1] == 'a' && cmd[i + 2] == 't')
	{
		i += 3;
		while (i < len && cmd[i] == ' ')
			i++;

		if(getqname(cmd, i)!=0)
		solveCat(fat12);
	}
}
//查询的目录里面没有空格 只有/和大写英文字母和数字
//1返回没有-l 2返回有-l
int getqname(const char *q, int st)
{
	int len = strlen(q);
	int dot = 0;
	int j = st;
	int qlen = 0;
	bool ls = 0;
	for (; j < len; j++)
	{
		if (q[j] == '.')
		{
			if (dot == 0)
				dot = 1;
			else //如果出现两个dot
			{
					my_print(warn2, strlen(warn2), 1);//lshoum 
				return 0;
			}
		}
		else if (q[j] == ' ') //空格的话 要么是非法字符 要么是目录已经结束
			break;
		else if (checkLetter(q[j]) == -1) //该字母是小写字母
		{
				my_print(warn3, strlen(warn3), 1);//lshoum 
			return 0;
		}
		else if (checkLetter(q[j]) == 0) //非法字符
		{
			my_print(warn2, strlen(warn2), 1);//lshoum 
			return 0;
		}
		Qfroot[qlen++] = q[j];
	}
	Qfroot[qlen] = '\0';
	//此时q[j]为空格
	while (j < len && q[j] == ' ') //过滤空格
		j++;
	if (j == len)
		return 2;
	//检查后面是否是 -lll 不是就是非法的
	for (; j < len; j++)
	{
		if (j < len && q[j] == '-')
		{
			ls = 1;
			if (j < len && q[j + 1] == 'l')
			{
				j++;
				while (j < len && q[j] == 'l')
				{
					j++;
				}
				while (j < len && q[j] == ' ')
					j++;
				if (j == len)
					return 1;
				else //-llll结束后面还有字符
				{
					my_print(warn2, strlen(warn2), 1);//lshoum 
					return 0;
				}
			}
			else //-后面不是l
			{
				my_print(warn2, strlen(warn2), 1);//lshoum 
				return 0;
			}
		}
		else //空格后面不是-
		{
					my_print(warn2, strlen(warn2), 1);//lshoum Z
			return 0;
		}
	}
	if (ls)
		return 1;
	else
		return 2;
}
void lsr(FILE *fat12)
{
	bool isFind = 0;
	for (auto ele : ftree)
	{
		char tmp[100];
		memset(tmp, 0, sizeof(tmp));
		strcat(tmp, ele.rname);
		strcat(tmp, ele.fname);
		if (strncmp(tmp, Qfroot, strlen(Qfroot)) == 0)
		{
			isFind = 1;
			pair<int, int> ans = solveRootFile(ele, fat12);
			my_print(tmp, strlen(tmp), 0);
			my_print(symb, strlen(symb), 0);
			my_print(newLine, strlen(newLine), 0);//lshoum 
			my_print(dot1, strlen(dot1), 1);//lshoum 
			for (auto obj : vcans)
			{
				my_print(obj.fname, strlen(obj.fname), obj.flag);//lshoum 
				my_print(blankSpace, strlen(blankSpace), 0);//lshoum 
			}
		   my_print(newLine, strlen(newLine), 0);//lshoum 
		}
	}
	if (isFind == 0)
	{
	    my_print(warn1, strlen(warn1), 0);//lshoum 
	}
}
void lslr(FILE *fat12)
{
	bool isFind = 0;
	for (auto ele : ftree)
	{
		char tmp[100];
		memset(tmp, 0, sizeof(tmp));
		strcat(tmp, ele.rname);
		strcat(tmp, ele.fname);
		char itmp[100];
		if (strncmp(tmp, Qfroot, strlen(Qfroot)) == 0)
		{
			isFind = 1;
			pair<int, int> ans = solveRootFile(ele, fat12);
			my_print(tmp, strlen(tmp), 0);
			my_print(split, strlen(split), 0);
			sprintf(itmp, "%d", ans.first);
			my_print(itmp, strlen(itmp), 0);
			my_print(blankSpace, strlen(blankSpace), 0);
			sprintf(itmp, "%d", ans.second);
			my_print(itmp, strlen(itmp), 0);
			my_print(colon, strlen(colon), 0);
			my_print(newLine, strlen(newLine), 0);

			my_print(dot2, strlen(dot2), 1);
			for (auto obj : vcans)
			{
				
				if (obj.flag)
				{
					my_print(obj.fname, strlen(obj.fname), 1);
					my_print(blankSpace, strlen(blankSpace), 0);
					sprintf(itmp, "%d", obj.cntr);
					my_print(itmp, strlen(itmp), 0);
					my_print(blankSpace, strlen(blankSpace), 0);
					sprintf(itmp, "%d", obj.cntf);
					my_print(itmp, strlen(itmp), 0);
					my_print(blankSpace, strlen(blankSpace), 0);
					my_print(newLine, strlen(newLine), 0);
				}
				else
				{
					my_print(obj.fname, strlen(obj.fname), 0);
					my_print(blankSpace, strlen(blankSpace), 0);
					sprintf(itmp, "%d", obj.fsize);
					my_print(itmp, strlen(itmp), 0);
					my_print(newLine, strlen(newLine), 0);
				}
				// my_print(newLine, strlen(newLine), 0);
			}
		}
	}
	if (isFind == 0)
	{
		my_print(warn1, strlen(warn1), 0);
	}
}