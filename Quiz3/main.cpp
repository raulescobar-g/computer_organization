#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <vector>
#include <pwd.h>
#include <grp.h>
#include <time.h>

using namespace std;

vector<string> get_files(const string& dir_str){
	vector<string> _files;
	DIR *dir;
	struct dirent *ent;

	if ((dir = opendir (dir_str.c_str())) == NULL) {
		perror ("openning directory");
		exit(1);
	} 
	while ((ent = readdir (dir)) != NULL) {
		if (ent->d_name[0] != '.')  _files.push_back(ent->d_name);
	}
	
	return _files;
}

string get_permissions(struct stat& file_stat){
	string perms = "";
	
	if (S_ISDIR(file_stat.st_mode)) perms += "d";
	else if (S_ISREG(file_stat.st_mode)) perms += "-";
	else if (S_ISCHR(file_stat.st_mode)) perms += "c";
	else if (S_ISBLK(file_stat.st_mode)) perms += "b";
	else if (S_ISFIFO(file_stat.st_mode)) perms += "p";
	else if (S_ISLNK(file_stat.st_mode)) perms += "l";
	else if (S_ISSOCK(file_stat.st_mode)) perms += "s";
	else perms += "n";

	perms += (file_stat.st_mode & S_IRUSR) ? "r" : "-";
	perms += (file_stat.st_mode & S_IWUSR) ? "w" : "-";
	perms += (file_stat.st_mode & S_IXUSR) ? "x" : "-";
	perms += (file_stat.st_mode & S_IRGRP) ? "r" : "-";
	perms += (file_stat.st_mode & S_IWGRP) ? "w" : "-";
	perms += (file_stat.st_mode & S_IXGRP) ? "x" : "-";
	perms += (file_stat.st_mode & S_IROTH) ? "r" : "-";
	perms += (file_stat.st_mode & S_IWOTH) ? "w" : "-";
	perms += (file_stat.st_mode & S_IXOTH) ? "x" : "-";
	return perms;
}

string date_parser(struct tm * mod) {
	time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    tstruct.tm_mon - mod->tm_mon >= 6 ? strftime(buf, sizeof(buf), "%b %e  %Y", mod): strftime(buf, sizeof(buf), "%b %e %H:%M", mod);
    return buf;
}

vector<string> get_file_data(const string& path, const string& file){
	int ret, links, filesize, tot_filesize;
	string permissions, mod;
	struct stat file_stat;
	struct passwd * user;
	struct group * group;
	vector<string> line;
	bool dir;

	string full_file = path + "/" + file;

	if((ret = lstat(full_file.c_str(), &file_stat)) < 0){ //open dir
		perror("Incorrect path or file status");
		exit(1);
	}

	dir = S_ISDIR(file_stat.st_mode);
	line.push_back(get_permissions(file_stat));
	line.push_back(to_string(file_stat.st_nlink));
	line.push_back(getpwuid(file_stat.st_uid)->pw_name);
	line.push_back(getgrgid(file_stat.st_gid)->gr_name);
	line.push_back(to_string(file_stat.st_size));
	dir ? line.push_back(date_parser(localtime(&(file_stat.st_ctime)))) : line.push_back(date_parser(localtime(&(file_stat.st_mtime))));
	line.push_back(file);

	if (S_ISLNK(file_stat.st_mode)){
		char buf[PATH_MAX];
		readlink(full_file.c_str(), buf, sizeof(buf));
		string link = "-> " + (string) buf;
		line.push_back(link);
	}

	return line;
}

vector<int> spacing(vector< vector<string> >& m){
	// first we determine the size each token takes up in the first line
	vector<int> spacing;
	for (const string& s : m[0]){
		spacing.push_back(s.size());
	}
	// we look for bigger spacing O(n^2) but its only like 5 tokens per line 
	for (const vector<string>& line : m){
		for (int i = 1; i < line.size()-1; ++i){
			if (line[i].size() > spacing[i]) spacing[i] = line[i].size();
		}
	}
	return spacing;
}

void print_matrix(vector<vector<string> > m, vector<int> s){
	for (vector<string>& line : m){
		cout << line[0]<< "  ";
		for (int i = 1; i < line.size()-1; ++i){
			for (int j = line[i].size(); j < s[i]; ++j){
				cout<<" ";
			}
			i == 1 || i == 4 || i == 5? cout<<line[i]<<" " : cout<<line[i]<<"  ";
		}
		cout<<line[line.size()-1]<<endl;
	}
}

int main(int argc, char *argv[]){
	// init to default parameters
	string path = "./";
    int opt;
	vector< vector<string> > ls_matrix;
	// get parameters 
	while ((opt = getopt(argc, argv, "l:")) != -1) {
		switch (opt) {
			case 'l':
				path = (string) optarg;
				break;
			default:
                cout<<"Wrong input"<<endl;
                exit(1);
		}
	}

	//getting all files in specified directory
	vector<string> files = get_files(path); 

	// iterating over file and appending file data into matrix of strings
	for (string& file : files){
		ls_matrix.push_back(get_file_data(path,file));
	}

	// determine spacing for each row in string matrix
	vector<int> spaces = spacing(ls_matrix);

	// now we print the matrix as is
	print_matrix(ls_matrix, spaces);

    return 0;
}