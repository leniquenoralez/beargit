#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"

/* Implementation Notes:
 *
 * - Functions return 0 if successful, 1 if there is an error.
 * - All error conditions in the function description need to be implemented
 *   and written to stderr. We catch some additional errors for you in main.c.
 * - Output to stdout needs to be exactly as specified in the function description.
 * - Only edit this file (beargit.c)
 * - You are given the following helper functions:
 *   * fs_mkdir(dirname): create directory <dirname>
 *   * fs_rm(filename): delete file <filename>
 *   * fs_mv(src,dst): move file <src> to <dst>, overwriting <dst> if it exists
 *   * fs_cp(src,dst): copy file <src> to <dst>, overwriting <dst> if it exists
 *   * write_string_to_file(filename,str): write <str> to filename (overwriting contents)
 *   * read_string_from_file(filename,str,size): read a string of at most <size> (incl.
 *     NULL character) from file <filename> and store it into <str>. Note that <str>
 *     needs to be large enough to hold that string.
 *  - You NEED to test your code. The autograder we provide does not contain the
 *    full set of tests that we will run on your code. See "Step 5" in the homework spec.
 */

/* beargit init
 *
 * - Create .beargit directory
 * - Create empty .beargit/.index file
 * - Create .beargit/.prev file containing 0..0 commit id
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_init(void) {
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);
  
  write_string_to_file(".beargit/.prev", "0000000000000000000000000000000000000000");

  return 0;
}


/* beargit add <filename>
 * 
 * - Append filename to list in .beargit/.index if it isn't in there yet
 *
 * Possible errors (to stderr):
 * >> ERROR: File <filename> already added
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_add(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      fprintf(stderr, "ERROR: File %s already added\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}


/* beargit rm <filename>
 * 
 * See "Step 2" in the homework 1 spec.
 *
 */

int beargit_rm(const char* filename) {

  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");
  int file_exist = 0;

  char line[FILENAME_SIZE];

  while(fgets(line, sizeof(line), findex)) {

    strtok(line, "\n");

    if (strcmp(line, filename) != 0) {

      fprintf(fnewindex, "%s\n", line);
    }else if(strcmp(line, filename) == 0){
  
      file_exist = 1;
    }

  }
  
  if (file_exist == 0)
  {
    fprintf(stderr, "ERROR: File %s not tracked\n", filename);
    return 1;
  }

  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.index", ".beargit/.index-old");
  fs_mv(".beargit/.newindex", ".beargit/.index");
  fs_rm(".beargit/.index-old");


  return 0;
}

/* beargit commit -m <msg>
 *
 * See "Step 3" in the homework 1 spec.
 *
 */

const char* go_bears = "GO BEARS!";

int is_commit_msg_ok(const char* msg) {

  if (strstr(msg, go_bears) != NULL){
    return 1;
  }
  return 0;
}

void next_commit_id(char* commit_id) {

  if (commit_id[0] == '0'){

    for (int i = 0; commit_id[i] != '\0'; ++i){

      commit_id[i] = '6';

    }

  }else{

    int inc = 0;
    int i = 0;

    while( inc != 1){

      if (commit_id[i] == '6'){

        commit_id[i] = '1';
        inc++;

      }else if(commit_id[i] == '1'){

        commit_id[i] = 'c';
        inc++;

      }else{

        i++;

      }
      
    }

  }

}
void new_commit_dir(char* filename){

  char newDir[50];

  strcpy(newDir, ".beargit/");
  strcat(newDir, filename);

  fs_mkdir(newDir);

}

void move_commit_file(char * file_dir, char * file, char * file_src ){


  int dir_name_len = 1 + strlen(file_dir) + strlen(file);

  char dir[dir_name_len];
  strcpy(dir, file_dir);

  if (file[0] != '/'){

    strcat(dir, "/");

  }

  strcat(dir, file);

  fs_cp(file_src,dir);

}

int beargit_commit(const char* msg) {

  if (!is_commit_msg_ok(msg)) {

    fprintf(stderr, "ERROR: Message must contain \"%s\"\n", go_bears);
    return 1;

  }


  // Generate New Commit ID
  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  next_commit_id(commit_id);

  // Create New Directory for current commit
  new_commit_dir(commit_id);

  char commit_dir[50];

  strcpy(commit_dir, ".beargit/");
  strcat(commit_dir, commit_id);

  // Copy .beargit/.index, .beargit/.prev and all tracked files into the directory.
  move_commit_file(commit_dir, "/.index" , ".beargit/.index");
  move_commit_file(commit_dir, "/.prev" ,".beargit/.prev");

  // Copy Commit Message
  // TODO: abstract to function to make cleaner
  char msg_file[55];
  strcpy(msg_file, commit_dir);
  strcat(msg_file, "/.msg");
  write_string_to_file(msg_file, msg);


  // Copy all files to current commit folder
  FILE* findex = fopen(".beargit/.index", "r");
  char line[FILENAME_SIZE];

  while(fgets(line, FILENAME_SIZE, findex)) {

    strtok(line, "\n");
    move_commit_file(commit_dir, line, line);
    
  }

  fclose(findex);



  // copy commit id to .beargit/.prev
  write_string_to_file(".beargit/.prev", commit_id);


  return 0;
}

/* beargit status
 *
 * See "Step 1" in the homework 1 spec.
 *
 */

int beargit_status() {

  FILE* findex = fopen(".beargit/.index", "r");
  char line[FILENAME_SIZE];
  int fileCount = 0;
  printf("Tracked files:\n");

  while(fgets(line, FILENAME_SIZE, findex)) {

    printf("%s\n", line);
    fileCount+=1;

  }

  printf("%d files total\n", fileCount);
  fclose(findex);

  return 0;

}

/* beargit log
 *
 * See "Step 4" in the homework 1 spec.
 *
 */

void get_commit_info(char* commit_dir, char* info, char* info_dst, int info_size){

  char file_path_len = strlen(info) + strlen(commit_dir);
  char file_path[file_path_len];

  strcpy(file_path, commit_dir);
  strcat(file_path, info);

  char commit_info[info_size];

  read_string_from_file(file_path, commit_info, info_size);

  strcpy(info_dst, commit_info);
  
}

int get_commit_directory(char* commit_id, char* file_name_dst){

  if (strcmp(commit_id,"0000000000000000000000000000000000000000") == 0){
    return 0;
  }

  char file_path[50] = ".beargit/";
  strcat(file_path, commit_id);

  strcpy(file_name_dst, file_path);

  return 1;
}


int beargit_log() {

  char prev_commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", prev_commit_id, COMMIT_ID_SIZE);

  if ( strcmp(prev_commit_id,"0000000000000000000000000000000000000000") == 0)
  {
    fprintf(stderr, "ERROR: There are no commits!\n");

    return 0;
  }

  while(strcmp(prev_commit_id,"0000000000000000000000000000000000000000") != 0){

    // Get dir for commit .beargit/commit_id
    char commit_dir[50]; 
    char commit_message[MSG_SIZE];
    get_commit_directory(prev_commit_id,commit_dir);
    get_commit_info(commit_dir,"/.msg", commit_message, MSG_SIZE);

    printf("\n");
    printf("commit %s\n", prev_commit_id);
    printf("\t%s",commit_message);

    get_commit_info(commit_dir,"/.prev", prev_commit_id, COMMIT_ID_SIZE);

  }

  return 0;
}
