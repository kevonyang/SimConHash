#include "stdafx.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <map>
#include <time.h>
using namespace std;
typedef unsigned int uint;

#define CONHASH_MAX_KEY_LENGTH 128
#define CONHASH_MAX_VNODE_NUM 99

//uint hash(uint x)
//{
//	x = ((x >> 16) ^ x) * 0x45d9f3b;
//	x = ((x >> 16) ^ x) * 0x45d9f3b;
//	x = (x >> 16) ^ x;
//	return x;
//}
//
//uint unhash(uint x) 
//{
//	x = ((x >> 16) ^ x) * 0x119de1f3;
//	x = ((x >> 16) ^ x) * 0x119de1f3;
//	x = (x >> 16) ^ x;
//	return x;
//}

// BKDR Hash Function
uint BKDRHash(char *str)
{
	uint seed = 13131; // 31 131 1313 13131 131313 etc..
	uint hash = 0;

	while (*str)
	{
		hash = hash * seed + (*str++);
	}

	return (hash & 0x7FFFFFFF);
}
struct HashNode
{
	char key[CONHASH_MAX_KEY_LENGTH];
	uint vNum;
};
struct VHashNode {
	uint hash;
	HashNode *node;
};

class ConHashMgr
{
public:
	static ConHashMgr *Instance()
	{
		if (!_inst)
		{
			_inst = new ConHashMgr();
		}
		return _inst;
	}

	HashNode *CreateNode(char *key, uint vNum)
	{
		if (strlen(key) > CONHASH_MAX_KEY_LENGTH - 1 || vNum > CONHASH_MAX_VNODE_NUM) return NULL;

		HashNode *node = new HashNode();
		_snprintf(node->key, CONHASH_MAX_KEY_LENGTH, "%s", key);
		node->vNum = vNum;
	
		return node;
	}

	void AddNode(HashNode *node)
	{
		static char cacheKey[CONHASH_MAX_KEY_LENGTH];
		_nodeMap.insert(std::make_pair(node->key, node));
		for (int i = 0; i < node->vNum; ++i)
		{
			_snprintf(cacheKey, CONHASH_MAX_KEY_LENGTH, "%s@%d", node->key, i);
			VHashNode *vnode = new VHashNode();
			vnode->hash = BKDRHash(cacheKey);
			vnode->node = node;
			_vNodeMap.insert(std::make_pair(vnode->hash, vnode));
		}
	}

	void RemoveNode(char *key)
	{
		auto it = _nodeMap.find(key);
		if (it == _nodeMap.end()) return;
		HashNode *node = it->second;
		static char cacheKey[CONHASH_MAX_KEY_LENGTH];
		for (int i = 0; i < node->vNum; ++i)
		{
			_snprintf(cacheKey, CONHASH_MAX_KEY_LENGTH, "%s@%d", node->key, i);
			uint hash = BKDRHash(cacheKey);
			auto vit = _vNodeMap.find(hash);
			if (vit != _vNodeMap.end())
			{
				VHashNode *vnode = vit->second;
				_vNodeMap.erase(vit);
				delete vnode;
			}
		}

		_nodeMap.erase(it);
		delete node;
	}

	HashNode *GetConHashNode(char *key)
	{
		if (_vNodeMap.empty()) return NULL;

		uint hash = BKDRHash(key);
		auto it = _vNodeMap.lower_bound(hash);

		if (it == _vNodeMap.end())
			it = _vNodeMap.begin();

		VHashNode *vnode = it->second;
		if (vnode == NULL)
			return NULL;

		return vnode->node;
	}

private:
	static ConHashMgr *_inst;
	map<uint, VHashNode*> _vNodeMap;
	map<char*, HashNode*> _nodeMap;
};

ConHashMgr *ConHashMgr::_inst = NULL;

int _tmain(int argc, _TCHAR* argv[])
{
	ConHashMgr *mgr = ConHashMgr::Instance();
	HashNode *node[3];
	node[0] = mgr->CreateNode("127.0.0.1:88", 64);
	mgr->AddNode(node[0]);
	node[1] = mgr->CreateNode("127.0.0.1:90", 64);
	mgr->AddNode(node[1]);
	node[2] = mgr->CreateNode("127.0.0.1:100", 64);
	mgr->AddNode(node[2]);

	int num[3] = { 0, 0, 0 };
	srand(time(0));
	for (int i = 1; i < 1000; ++i)
	{
		static char buff[CONHASH_MAX_KEY_LENGTH];
		_snprintf(buff, CONHASH_MAX_KEY_LENGTH, "rand%d", rand() % 10000);
		HashNode *conNode = mgr->GetConHashNode(buff);
		printf("conNode,%s,%s\n", buff, conNode->key);
		for (int j = 0; j < 3; ++j)
		{
			if (node[j] == conNode)
				++num[j];
		}
	}
	printf("nodeNum,%d,%d,%d\n", num[0], num[1], num[2]);

	mgr->RemoveNode("127.0.0.1:88");
	mgr->RemoveNode("127.0.0.1:90");
	mgr->RemoveNode("127.0.0.1:99");
	return 0;
	
}

