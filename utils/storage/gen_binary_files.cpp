/* 
 *  libpinyin
 *  Library to deal with pinyin.
 *  
 *  Copyright (C) 2010 Peng Wu
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdio.h>
#include <locale.h>
#include "pinyin.h"

void print_help(){
    printf("Usage: gen_binary_files --table-dir <DIRNAME>\n");
}

int main(int argc, char * argv[]){
    int i = 1;
    const char * table_dir = ".";

    setlocale(LC_ALL, "");
    while ( i < argc ){
        if ( strcmp("--help", argv[i]) == 0 ){
            print_help();
            exit(0);
        } else if ( strcmp("--table-dir", argv[i]) == 0){
            if ( ++i >= argc ){
                print_help();
                exit(EINVAL);
            }
            table_dir = argv[i];
        }
        ++i;
    }

    /* generate pinyin index*/
    PinyinCustomSettings custom;
    PinyinLargeTable pinyinlargetable(&custom);
    PhraseLargeTable phraselargetable;

    /* generate phrase index */
    FacadePhraseIndex phrase_index;

    gchar * filename = g_build_filename(table_dir, "gb_char.table", NULL);
    FILE * gbfile = fopen(filename, "r");
    g_free(filename);

    if ( gbfile == NULL) {
	fprintf(stderr, "open gb_char.table failed!");
	exit(ENOENT);
    }

    pinyinlargetable.load_text(gbfile);

    fseek(gbfile, 0L, SEEK_SET);
    phraselargetable.load_text(gbfile);
    fseek(gbfile, 0L, SEEK_SET);
    phrase_index.load_text(1, gbfile);
    fclose(gbfile);

    filename = g_build_filename(table_dir, "gbk_char.table", NULL);
    FILE * gbkfile = fopen(filename, "r");
    g_free(filename);

    if ( gbkfile == NULL) {
        fprintf(stderr, "open gbk_char.table failed!");
        exit(ENOENT);
    }
    
    pinyinlargetable.load_text(gbkfile);

    fseek(gbkfile, 0L, SEEK_SET);
    phraselargetable.load_text(gbkfile);
    fseek(gbkfile, 0L, SEEK_SET);
    phrase_index.load_text(2, gbkfile);
    fclose(gbkfile);

    MemoryChunk * new_chunk = new MemoryChunk;
    pinyinlargetable.store(new_chunk);
    new_chunk->save("pinyin_index.bin");
    pinyinlargetable.load(new_chunk);
    
    new_chunk = new MemoryChunk;
    phraselargetable.store(new_chunk);
    new_chunk->save("phrase_index.bin");
    phraselargetable.load(new_chunk);

    phrase_index.compat();

    new_chunk = new MemoryChunk;
    phrase_index.store(1, new_chunk);
    new_chunk->save("gb_char.bin");
    phrase_index.load(1, new_chunk);

    new_chunk = new MemoryChunk;
    phrase_index.store(2, new_chunk);
    new_chunk->save("gbk_char.bin");
    phrase_index.load(2, new_chunk);
    
    return 0;
}
