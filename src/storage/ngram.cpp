/* 
 *  libpinyin
 *  Library to deal with pinyin.
 *  
 *  Copyright (C) 2006-2007 Peng Wu
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
#include <glib.h>
#include <glib/gstdio.h>
#include "memory_chunk.h"
#include "novel_types.h"
#include "ngram.h"

using namespace pinyin;

struct SingleGramItem{
    phrase_token_t m_token;
    guint32 m_freq;
};

SingleGram::SingleGram(){
    m_chunk.set_size(sizeof(guint32));
    memset(m_chunk.begin(), 0, sizeof(guint32));
}

SingleGram::SingleGram(void * buffer, size_t length){
    m_chunk.set_chunk(buffer, length, NULL);
}

bool SingleGram::get_total_freq(guint32 & total){
    char * buf_begin = (char *)m_chunk.begin();
    total = *((guint32 *)buf_begin);
    return true;
}

bool SingleGram::set_total_freq(guint32 total){
    char * buf_begin = (char *)m_chunk.begin();
    *((guint32 *)buf_begin) = total;
    return true;
}

bool SingleGram::prune(){
    assert(false);
#if 0
    SingleGramItem * begin = (SingleGramItem *)
        ((const char *)(m_chunk.begin()) + sizeof(guint32));
    SingleGramItem * end = (SingleGramItem *)m_chunk.end();
    
    size_t nitem = 0;
    for ( SingleGramItem * cur = begin; cur != end; ++cur){
        cur->m_freq--;
        nitem++;
        if ( cur->m_freq == 0 ){
            size_t offset = sizeof(guint32) + (cur - begin)
                * sizeof(SingleGramItem) ;
            m_chunk.remove_content(offset, sizeof(SingleGramItem));
        }
    }
    guint32 total_freq;
    assert(get_total_freq(total_freq));
    assert(set_total_freq(total_freq - nitem));
#endif
        return true;
}

static bool token_less_than(const SingleGramItem & lhs,const SingleGramItem & rhs){
    return lhs.m_token < rhs.m_token;
}

bool SingleGram::retrieve_all(/* out */ BigramPhraseWithCountArray array){
    const SingleGramItem * begin = (const SingleGramItem *)
        ((const char *)(m_chunk.begin()) + sizeof(guint32));
    const SingleGramItem * end = (const SingleGramItem *) m_chunk.end();

    guint32 total_freq;
    BigramPhraseItemWithCount bigram_item_with_count;
    assert(get_total_freq(total_freq));

    for ( const SingleGramItem * cur_item = begin; cur_item != end; ++cur_item){
        bigram_item_with_count.m_token = cur_item->m_token;
        bigram_item_with_count.m_count = cur_item->m_freq;
        bigram_item_with_count.m_freq = cur_item->m_freq / (gfloat)total_freq;
        g_array_append_val(array, bigram_item_with_count);
    }

    return true;
}

bool SingleGram::search(/* in */ PhraseIndexRange * range, 
                        /* out */ BigramPhraseArray array){
    const SingleGramItem * begin = (const SingleGramItem *)
        ((const char *)(m_chunk.begin()) + sizeof(guint32));
    const SingleGramItem * end = (const SingleGramItem *)m_chunk.end();

    SingleGramItem compare_item;
    compare_item.m_token = range->m_range_begin;
    const SingleGramItem * cur_item = std_lite::lower_bound(begin, end, compare_item, token_less_than);

    guint32 total_freq;
    BigramPhraseItem bigram_item;
    assert(get_total_freq(total_freq));

    for ( ; cur_item != end; ++cur_item){
        if ( cur_item->m_token >= range->m_range_end )
            break;
        bigram_item.m_token = cur_item->m_token;
        bigram_item.m_freq = cur_item->m_freq / (gfloat)total_freq;
        g_array_append_val(array, bigram_item);
    }

    return true;
}

bool SingleGram::insert_freq( /* in */ phrase_token_t token,
                              /* in */ guint32 freq){
    SingleGramItem * begin = (SingleGramItem *)
        ((const char *)(m_chunk.begin()) + sizeof(guint32));
    SingleGramItem * end = (SingleGramItem *) m_chunk.end();
    SingleGramItem compare_item;
    compare_item.m_token = token;
    SingleGramItem * cur_item = std_lite::lower_bound(begin, end, compare_item, token_less_than);

    SingleGramItem insert_item;
    insert_item.m_token = token;
    insert_item.m_freq = freq;
    for ( ; cur_item != end; ++cur_item ){
        if ( cur_item->m_token > token ){
            size_t offset = sizeof(guint32) +
                sizeof(SingleGramItem) * (cur_item - begin);
            m_chunk.insert_content(offset, &insert_item,
                                   sizeof(SingleGramItem));
            return true;
        }
        if ( cur_item->m_token == token ){
            return false;
        }
    }
    m_chunk.insert_content(m_chunk.size(), &insert_item,
                           sizeof(SingleGramItem));
    return true;
}

bool SingleGram::remove_freq( /* in */ phrase_token_t token,
                              /* out */ guint32 & freq){
    freq = 0;
    const SingleGramItem * begin = (const SingleGramItem *)
        ((const char *)(m_chunk.begin()) + sizeof(guint32));
    const SingleGramItem * end = (const SingleGramItem *)m_chunk.end();
    SingleGramItem compare_item;
    compare_item.m_token = token;
    const SingleGramItem * cur_item = std_lite::lower_bound(begin, end, compare_item, token_less_than);

    for ( ; cur_item != end; ++cur_item ){
        if ( cur_item->m_token > token )
            return false;
        if ( cur_item->m_token == token ){
            freq = cur_item -> m_freq;
            size_t offset = sizeof(guint32) +
                sizeof(SingleGramItem) * (cur_item - begin);
            m_chunk.remove_content(offset, sizeof(SingleGramItem));
            return true;
        }
    }
    return false;
}

bool SingleGram::get_freq(/* in */ phrase_token_t token,
                          /* out */ guint32 & freq){
    freq = 0;
    const SingleGramItem * begin = (const SingleGramItem *)
        ((const char *)(m_chunk.begin()) + sizeof(guint32));
    const SingleGramItem * end = (const SingleGramItem *)m_chunk.end();
    SingleGramItem compare_item;
    compare_item.m_token = token;
    const SingleGramItem * cur_item = std_lite::lower_bound(begin, end, compare_item, token_less_than);
    
    for ( ; cur_item != end; ++cur_item){
        if ( cur_item->m_token > token )
            return false;
        if ( cur_item->m_token == token ){
            freq = cur_item -> m_freq;
            return true;
        }
    }
    return false;
}

bool SingleGram::set_freq( /* in */ phrase_token_t token,
                           /* in */ guint32 freq){
    SingleGramItem * begin = (SingleGramItem *)
        ((const char *)(m_chunk.begin()) + sizeof(guint32));
    SingleGramItem * end = (SingleGramItem *)m_chunk.end();
    SingleGramItem compare_item;
    compare_item.m_token = token;
    SingleGramItem * cur_item = std_lite::lower_bound(begin, end, compare_item, token_less_than);
    
    for ( ;cur_item != end; ++cur_item){
        if ( cur_item->m_token > token ){
            return false;
        }
        if ( cur_item->m_token == token ){
            cur_item -> m_freq = freq;
            return true;
        }
    }
    return false;
}

bool Bigram::load_db(const char * dbfile){
    reset();

    DB * tmp_db = NULL;
    int ret = db_create(&tmp_db, NULL, 0);
    assert(ret == 0);

    ret = tmp_db->open(tmp_db, NULL, dbfile, NULL,
                       DB_HASH, DB_RDONLY, 0600);
    if ( ret != 0 )
        return false;

    ret = db_create(&m_db, NULL, 0);
    assert(ret == 0);

    ret = m_db->open(m_db, NULL, NULL, NULL,
                     DB_HASH, DB_CREATE, 0600);
    if ( ret != 0 )
        return false;

    DBC * cursorp = NULL;
    DBT key, data;
    /* Get a cursor */
    tmp_db->cursor(tmp_db, NULL, &cursorp, 0);

    /* Initialize our DBTs. */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    /* Iterate over the database, retrieving each record in turn. */
    while ((ret = cursorp->c_get(cursorp, &key, &data, DB_NEXT)) == 0) {
        int ret = m_db->put(m_db, NULL, &key, &data, 0);
        assert(ret == 0);
    }
    assert (ret == DB_NOTFOUND);

    /* Cursors must be closed */
    if ( cursorp != NULL )
        cursorp->c_close(cursorp);

    if ( tmp_db != NULL )
        tmp_db->close(tmp_db, 0);

    return true;
}

bool Bigram::save_db(const char * dbfile){
    DB * tmp_db = NULL;

    int ret = g_unlink(dbfile);
    if ( ret != 0 )
        return false;

    ret = db_create(&tmp_db, NULL, 0);
    assert(ret == 0);

    ret = tmp_db->open(tmp_db, NULL, dbfile, NULL,
                       DB_HASH, DB_CREATE, 0600);
    if ( ret != 0 )
        return false;

    DBC * cursorp = NULL;
    DBT key, data;
    /* Get a cursor */
    m_db->cursor(m_db, NULL, &cursorp, 0);

    /* Initialize our DBTs. */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));

    /* Iterate over the database, retrieving each record in turn. */
    while ((ret = cursorp->c_get(cursorp, &key, &data, DB_NEXT)) == 0) {
        int ret = tmp_db->put(tmp_db, NULL, &key, &data, 0);
        assert(ret == 0);
    }
    assert (ret == DB_NOTFOUND);

    /* Cursors must be closed */
    if ( cursorp != NULL )
        cursorp->c_close(cursorp);

    if ( tmp_db != NULL )
        tmp_db->close(tmp_db, 0);

    return true;
}

bool Bigram::attach(const char * dbfile, guint32 flags){
    reset();
    u_int32_t db_flags = 0;

    if ( flags & ATTACH_READONLY )
        db_flags |= DB_RDONLY;
    if ( flags & ATTACH_READWRITE )
        assert( !( flags & ATTACH_READONLY ) );
    if ( flags & ATTACH_CREATE )
        db_flags |= DB_CREATE;

    if ( !dbfile )
        return false;
    int ret = db_create(&m_db, NULL, 0);
    if ( ret != 0 )
        assert(false);
        
    ret = m_db->open(m_db, NULL, dbfile, NULL,
                     DB_HASH, db_flags, 0644);
    if ( ret != 0)
        return false;

    return true;
}

bool Bigram::load(phrase_token_t index, SingleGram * & single_gram){
    single_gram = NULL;
    if ( !m_db )
        return false;

    DBT db_key;
    memset(&db_key, 0, sizeof(DBT));
    db_key.data = &index;
    db_key.size = sizeof(phrase_token_t);

    DBT db_data;
    memset(&db_data, 0, sizeof(DBT));
    int ret = m_db->get(m_db, NULL, &db_key, &db_data, 0);
    if ( ret != 0 )
        return false;

    single_gram = new SingleGram(db_data.data, db_data.size);
    return true;
}

bool Bigram::store(phrase_token_t index, SingleGram * single_gram){
    if ( !m_db )
        return false;

    DBT db_key;
    memset(&db_key, 0, sizeof(DBT));
    db_key.data = &index;
    db_key.size = sizeof(phrase_token_t);
    DBT db_data;
    memset(&db_data, 0, sizeof(DBT));
    db_data.data = single_gram->m_chunk.begin();
    db_data.size = single_gram->m_chunk.size();
    
    int ret = m_db->put(m_db, NULL, &db_key, &db_data, 0);
    return ret == 0;
}

bool Bigram::get_all_items(GArray * items){
    g_array_set_size(items, 0);

    if ( !m_db )
        return false;

    DBC * cursorp = NULL;
    DBT key, data;
    int ret;
    /* Get a cursor */
    m_db->cursor(m_db, NULL, &cursorp, 0); 

    /* Initialize our DBTs. */
    memset(&key, 0, sizeof(DBT));
    memset(&data, 0, sizeof(DBT));
        
    /* Iterate over the database, retrieving each record in turn. */
    while ((ret = cursorp->c_get(cursorp, &key, &data, DB_NEXT)) == 0) {
        assert(key.size == sizeof(phrase_token_t));
        phrase_token_t * token = (phrase_token_t *)key.data;
        g_array_append_val(items, *token);
    }

    assert (ret == DB_NOTFOUND);

    /* Cursors must be closed */
    if (cursorp != NULL) 
        cursorp->c_close(cursorp); 

    return true;
}
