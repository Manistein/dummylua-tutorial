/* Copyright (c) 2018 Manistein, https://manistein.github.io/blog/  

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.*/

#include "luatable.h"
#include "../vm/luagc.h"
#include "luamem.h"
#include "luastring.h"
#include "../vm/luavm.h"
#include "../vm/luado.h"
#include "luaobject.h"

#define MAXASIZE (1u << MAXABITS)

static int l_hashfloat(lua_Number n) {
    int i = 0;
    lua_Integer ni = 0;

    n = frexp(n, &i) * -cast(lua_Number, INT_MIN);
    if (!lua_numbertointeger(n, &ni)) {
        lua_assert(lua_numisnan(n) || fabs(n) == cast(lua_Number, HUGE_VAL));
        return 0;
    }
    else {
        unsigned int u = cast(unsigned int, ni) + cast(unsigned int, i);
        return u <= INT_MAX ? u : ~u;
    }
}

static Node* mainposition(struct lua_State* L, struct Table* t, const TValue* key) {
    switch(key->tt_) {
        case LUA_NUMINT: return hashint(key->value_.i, t); 
        case LUA_NUMFLT: return hashint(l_hashfloat(key->value_.n), t);
        case LUA_TBOOLEAN: return hashboolean(key->value_.b, t);
        case LUA_SHRSTR: return hashstr(gco2ts(gcvalue(key)), t);
        case LUA_LNGSTR: {
            struct TString* ts = gco2ts(gcvalue(key));
            luaS_hashlongstr(L, ts);
            return hashstr(ts, t);
        };
        case LUA_TLIGHTUSERDATA: {
            return hashpointer(key->value_.p, t);
        };
        case LUA_TLCF: {
            return hashpointer(key->value_.f, t);
        };
        default: {
            lua_assert(!ttisdeadkey(gcvalue(key))); 
            return hashpointer(gcvalue(key), t);
        };  
    } 
}

static const TValue* getgeneric(struct lua_State* L, struct Table* t, const TValue* key) {
    Node* n = mainposition(L, t, key); 
    for (;;) {
        const TValue* k = getkey(n);
        if (luaV_eqobject(L, k, key)) {
            return getval(n);
        }
        else {
            int next = n->key.nk.next;
            if (next == 0) {
                break;
            }
            n += next;
        }
    } 

    return luaO_nilobject;
}

static void setnodesize(struct lua_State* L, struct Table* t, int size) {
    if (size == 0) {
        t->lsizenode = 0;
        t->node = cast(Node*, dummynode);
        t->lastfree = NULL;
    }
    else {
        int lsize = luaO_ceillog2(size);
        t->lsizenode = (unsigned int)lsize;
        if (t->lsizenode > (sizeof(int) * CHAR_BIT - 1)) {
            luaD_throw(L, LUA_ERRRUN);
        }

        int node_size = twoto(lsize);
        t->node = (Node*)luaM_newvector(L, node_size, Node);
        t->lastfree = &t->node[node_size]; // it is not a bug, at the beginning, lastfree point to the address which next to last node
        
        for (int i = 0; i < node_size; i++) {
            Node* n = &t->node[i];
            TKey* k = &n->key;
            k->nk.next = 0;
            setnilvalue(getwkey(n));
            setnilvalue(getval(n));
        }
    }
}

struct Table* luaH_new(struct lua_State* L) {
    struct GCObject* o = luaC_newobj(L, LUA_TTABLE, sizeof(struct Table));
    struct Table* t = gco2tbl(o);
    t->array = NULL;
    t->arraysize = 0;
    t->node = NULL;
    t->lsizenode = 0;
    t->lastfree = NULL;
    t->gclist = NULL;
    
    setnodesize(L, t, 0);
    return t;
}

void luaH_free(struct lua_State* L, struct Table* t) {
    if (t->arraysize > 0) {
        luaM_free(L, t->array, t->arraysize * sizeof(TValue));
    }

    if (!isdummy(t)) {
        luaM_free(L, t->node, twoto(t->lsizenode) * sizeof(struct Node));
    }

    luaM_free(L, t, sizeof(struct Table));
}

const TValue* luaH_getint(struct lua_State* L, struct Table* t, lua_Integer key) {
    // 1 <= key <= arraysize
    if (cast(unsigned int, key) - 1 < t->arraysize) {
        return cast(const TValue*, &t->array[key - 1]);        
    }
    else {
        struct Node* n = hashint(key, t);
        while(true) {
            const TKey* k = &n->key;
            if (ttisinteger(&k->tvk) && k->tvk.value_.i == key) {
                return cast(const TValue*, getval(n));
            }
            else {
                int next = k->nk.next;
                if (next == 0) {
                    break;
                }
                n += next; 
            }
        }
    }

    return luaO_nilobject;
}

int luaH_setint(struct lua_State* L, struct Table* t, int key, const TValue* value) {
    const TValue* p = luaH_getint(L, t, key);
    TValue* cell = NULL;
    if (p != luaO_nilobject) {
        cell = cast(TValue*, p);
    }
    else {
        TValue k;
        setivalue(&k, key);
        cell = luaH_newkey(L, t, &k); 
    }
    
    cell->value_ = value->value_;
    cell->tt_ = value->tt_;
    return LUA_OK;
}

const TValue* luaH_getshrstr(struct lua_State* L, struct Table* t, struct TString* key) {
    lua_assert(key->tt_ == LUA_SHRSTR);
    struct Node* n = hashstr(key, t);
    for (;;) {
        TKey* k = &n->key;
        if (ttisshrstr(&k->tvk) && luaS_eqshrstr(L, gco2ts(gcvalue(&k->tvk)), key)) {
            return getval(n);
        }
        else {
            int next = k->nk.next;
            if (next == 0) {
                break;
            }
            n += next;
        }
    }

    return luaO_nilobject;
}

const TValue* luaH_getstr(struct lua_State* L, struct Table* t, struct TString* key) {
    if (key->tt_ == LUA_SHRSTR) {
        return luaH_getshrstr(L, t, key);
    }
    else {
        TValue k;
        setgco(&k, obj2gco(key));
        return getgeneric(L, t, &k);
    }
}

const TValue* luaH_get(struct lua_State* L, struct Table* t, const TValue* key) {
    switch(key->tt_) {
        case LUA_TNIL:   return luaO_nilobject;
        case LUA_NUMINT: return luaH_getint(L, t, key->value_.i); 
        case LUA_NUMFLT: return luaH_getint(L, t, l_hashfloat(key->value_.n));
        case LUA_SHRSTR: return luaH_getshrstr(L, t, gco2ts(gcvalue(key)));
        case LUA_LNGSTR: return luaH_getstr(L, t, gco2ts(gcvalue(key)));
        default:{
            return getgeneric(L, t, key);   
        };
    }
}

TValue* luaH_set(struct lua_State* L, struct Table* t, const TValue* key) {
    const TValue* p = luaH_get(L, t, key);
    if (p != luaO_nilobject) {
        return cast(TValue*, p); 
    }
    else {
        return luaH_newkey(L, t, key);
    }
}

typedef struct Auxnode {
    struct Table* t;
    unsigned int size;
} Auxnode;

static int aux_set_node_size(struct lua_State* L, void* ud) {
    struct Auxnode* n = cast(struct Auxnode*, ud);
    setnodesize(L, n->t, n->size);
    return 0;
}

int luaH_resize(struct lua_State* L, struct Table* t, unsigned int asize, unsigned int hsize) {
    unsigned int old_asize = t->arraysize;
    Node* old_node = t->node;
    unsigned int old_node_size = isdummy(t) ? 0 : twoto(t->lsizenode);

    if (asize > t->arraysize) {
        luaM_reallocvector(L, t->array, t->arraysize, asize, TValue); 
        t->arraysize = asize;

        for (unsigned int i = old_asize; i < asize; i ++) {
            setnilvalue(&t->array[i]);
        }
    }

    Auxnode auxnode;
    auxnode.t = t;
    auxnode.size = hsize;
    if (luaD_rawrunprotected(L, &aux_set_node_size, &auxnode) != LUA_OK) {
        luaM_reallocvector(L, t->array, t->arraysize, old_asize, TValue);
        luaD_throw(L, LUA_ERRRUN);
    }

    // shrink array
    if (asize < old_asize) {
        for (unsigned int i = asize; i < old_asize; i++) {
            if (!ttisnil(&t->array[i])) {
                luaH_setint(L, t, i + 1, &t->array[i]);
            }
        }
        luaM_reallocvector(L, t->array, old_asize, asize, TValue);
        t->arraysize = asize;
    }

    for (unsigned int i = 0; i < old_node_size; i++) {
        Node* n = &old_node[i];
        if (!ttisnil(getval(n))) {
            setobj(luaH_set(L, t, getkey(n)), getval(n));
        }
    }

    if (old_node_size > 0) {
        luaM_free(L, old_node, old_node_size * sizeof(Node));
    }

    return LUA_OK;
}

static Node* getlastfree(struct Table* t) {
    if (t->lastfree == NULL) {
        return NULL;
    }

    while(true) {
        if (t->lastfree == t->node) {
            break;
        }
        t->lastfree--;
        if (ttisnil(getval(t->lastfree))) {
            return t->lastfree;
        }
    }

    return NULL;
}

/***
 * nums[i]统计array 2 ^ (i - 1) ~ 2 ^ i区间内，不为nil的元素个数
 * 同时他也统计node列表中，key为lua_Integer类型，值不为nil的节点个数，
 * 会计算出node的key位于哪个区间中，并统计
 ***/
static int numsarray(struct Table* t, int* nums) {
    int totaluse = 0;
    int idx = 0;
    for (int i = 0, twotoi = 1; twotoi <= (int)t->arraysize; i ++, twotoi *= 2) {
        for (; idx < twotoi; idx++) {
            if (!ttisnil(&t->array[idx])) {
                totaluse ++;
                nums[i] ++;
            }
        }
    } 
    return totaluse;
}

static int numshash(struct Table* t, int* nums) {
    int totaluse = 0;
    for (int i = 0; i < twoto(t->lsizenode); i ++) {
        Node* n = &t->node[i];
        if ( !ttisnil(getval(n))) {
            totaluse ++;

            if (ttisinteger(getkey(n)) && getkey(n)->value_.i > 0) {
                lua_Integer ikey = getkey(n)->value_.i;
                int temp = luaO_ceillog2(ikey);

                if (temp < MAXABITS + 1 && temp >= 0)
                    nums[temp] ++;
            }
        }
    }
    return totaluse;
}

static int compute_array_size(struct Table* t, int* nums, int* array_used_num) {
    int array_size = 0;
    int sum_array_used = 0;
    int sum_int_keys = 0;
    for (int i = 0, twotoi = 1; (i < MAXABITS + 1) && (twotoi > 0); i++, twotoi *= 2) {
        sum_int_keys += nums[i];
        if (sum_int_keys > twotoi / 2) {
            array_size = twotoi;
            sum_array_used = sum_int_keys;
        }
    }
    *array_used_num = sum_array_used;
    return array_size;
}

//** nums[i] = number of keys 'k' where 2^(i - 1) < k <= 2^i 
static void rehash(struct lua_State* L, struct Table* t, const TValue* key) {
    int nums[MAXABITS + 1];
    for (int i = 0; i < MAXABITS + 1; i++) {
        nums[i] = 0;
    }

    int totaluse = 0;
    int array_used_num = 0;
    
    totaluse = numsarray(t, nums);
    totaluse += numshash(t, nums);

    totaluse ++;
    if (ttisinteger(key) && key->value_.i > 0) {
        int temp = luaO_ceillog2(key->value_.i);
        if (temp < MAXABITS - 1 && temp >= 0)
            nums[temp]++;
    }

    int asize = compute_array_size(t, nums, &array_used_num);
    luaH_resize(L, t, asize, totaluse - array_used_num);
}

TValue* luaH_newkey(struct lua_State* L, struct Table* t, const TValue* key) {
    if (ttisnil(key)) {
        luaD_throw(L, LUA_ERRRUN);
    }

    TValue k;
    if (ttisfloat(key)) {
        if (lua_numisnan(key->value_.n)) {
            luaD_throw(L, LUA_ERRRUN);
        }

        k.value_.i = l_hashfloat(key->value_.n);
        k.tt_ = LUA_NUMINT;
        key = &k;
    }

    Node* main_node = mainposition(L, t, key);
    if (!ttisnil(getval(main_node)) || isdummy(t)) {
        Node* lastfree = getlastfree(t); 
        if (lastfree == NULL) {
            rehash(L, t, key);
            return luaH_set(L, t, key);
        }

        Node* other_node = mainposition(L, t, getkey(main_node));
        if (other_node != main_node) {
            // find previous node of main node
            while(other_node + other_node->key.nk.next != main_node)
                other_node += other_node->key.nk.next;
            
            other_node->key.nk.next = lastfree - other_node;
            setobj(getwkey(lastfree), getwkey(main_node));
            setobj(getval(lastfree), getval(main_node));

            main_node->key.nk.next = 0;
            setnilvalue(getval(main_node));
        }
        else {			
			if (main_node->key.nk.next != 0) {
				Node* next = main_node + main_node->key.nk.next;
				lastfree->key.nk.next = next - lastfree;
			}
            main_node->key.nk.next = lastfree - main_node;
            main_node = lastfree;
        }
    }

    setobj(getwkey(main_node), cast(TValue*, key));
    luaC_barrierback(L, t, key);
    lua_assert(ttisnil(getval(main_node)));

    return getval(main_node);
}

static unsigned int arrayindex(struct Table* t, const TValue* key) {
    if (ttisinteger(key)) {
        lua_Integer k = key->value_.i;
        if ((k > 0) && (lua_Unsigned)k < MAXASIZE) {
            return cast(unsigned int, k);
        }
    }
    return 0;
}

static unsigned int findindex(struct lua_State* L, struct Table* t, const TValue* key) {
    unsigned int i = arrayindex(t, key);
    if (i != 0 && i <= t->arraysize) {
        return i;
    }
    else {
        Node* n = mainposition(L, t, key);
        while(true) {
            // lua允许在遍历的过程中，为table的域赋nil值，这样可能在迭代过程中
            // 遇到gc的情况，将刚刚遍历过的key设置为dead key，如果不处理这种情况
            // 那么迭代将会跳过下一个key，进入下下个key，导致遍历不到下一个key
            if (luaV_eqobject(L, getkey(n), key) || 
                    (ttisdeadkey(getkey(n)) && iscollectable(key) && (gcvalue(getkey(n)) == gcvalue(key)))) {
                i = n - getnode(t, 0);
                return (i + 1) + t->arraysize;
            }

            if (n->key.nk.next == 0) {
                luaD_throw(L, LUA_ERRRUN);
            }
            n += n->key.nk.next;
        }
    }
    return 0;
}

int luaH_next(struct lua_State* L, struct Table* t, TValue* key) {
    unsigned int i = findindex(L, t, key);
    for (; i < t->arraysize; i ++) {
        if (!ttisnil(&t->array[i])) {
            setivalue(key, i + 1);
            setobj(key + 1, &t->array[i]);
            return LUA_OK;
        }
    }
    
    for (i -= t->arraysize; i < (unsigned int)twoto(t->lsizenode); i ++) {
        Node* n = getnode(t, i);
        if (!ttisnil(getval(n))) {
            setobj(key, getwkey(n));
            setobj(key + 1, getval(n));
            return LUA_OK;
        }
    }
    return -1;
}

int luaH_getn(struct lua_State* L, struct Table* t) {
    int n = 0;
    for (int i = 0; i < (int)t->arraysize; i++) {
        if (!ttisnil(&t->array[i])) {
            n ++;
        }
        else {
            return n;
        }
    }
    return n;
}
