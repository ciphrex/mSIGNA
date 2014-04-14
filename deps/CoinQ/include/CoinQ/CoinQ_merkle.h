///////////////////////////////////////////////////////////////////////////////
//
// CoinQ_merkle.h 
//
// Copyright (c) 2013 Eric Lombrozo
//
// All Rights Reserved.

#ifndef _COINQ_MERKLE_H_
#define _COINQ_MERKLE_H_

#include <hash.h>

#include <string>
#include <sstream>
#include <stdexcept>
#include <assert.h>

template <class hash_T>
class CoinQMerkleNode
{
private:
    mutable hash_T hash;
    mutable bool bHashChanged;

    mutable unsigned int depth;
    mutable bool bDepthChanged;

    CoinQMerkleNode<hash_T>* pLeftChild;
    CoinQMerkleNode<hash_T>* pRightChild;

public:
    CoinQMerkleNode() : bHashChanged(false), depth(0), bDepthChanged(false), pLeftChild(NULL), pRightChild(NULL) { }
    CoinQMerkleNode(const hash_T& hash) : bHashChanged(false), depth(0), bDepthChanged(false), pLeftChild(NULL), pRightChild(NULL) { setHash(hash); }

    ~CoinQMerkleNode() { if (pLeftChild) delete pLeftChild; if (pRightChild) delete pRightChild; }

    void setHash(const hash_T& hash);
    const hash_T& getHash() const;

    unsigned int getDepth() const;

    unsigned int getNumChildren() const;
    bool insertLeaf(CoinQMerkleNode<hash_T>* pLeaf, int insertDepth = -1);

    std::string toString(const std::string& prefix = "") const; // prefix traversal
};

template <class hash_T>
void CoinQMerkleNode<hash_T>::setHash(const hash_T& hash)
{
    assert(!pLeftChild && !pRightChild);
    this->hash = hash;
}

template <class hash_T>
const hash_T& CoinQMerkleNode<hash_T>::getHash() const
{
    if (!bHashChanged) return hash;

    assert(pLeftChild);

    if (pRightChild) {
        hash = (pLeftChild->getHash() + pRightChild->getHash());
        hash = hash.hash();
    }
    else {
        hash = (pLeftChild->getHash() + pLeftChild->getHash());
        hash = hash.hash();
    }

    bHashChanged = false;
    return hash;
}

template <class hash_T>
unsigned int CoinQMerkleNode<hash_T>::getDepth() const
{
    if (!bDepthChanged) return depth;

    assert(pLeftChild);

    unsigned int leftDepth = pLeftChild ? pLeftChild->getDepth() + 1 : 0;
    unsigned int rightDepth = pRightChild ? pRightChild->getDepth() + 1 : 0;
    if (leftDepth > rightDepth) {
        depth = leftDepth;
    }
    else {
        depth = rightDepth;
    }

    bDepthChanged = false;
    return depth;
}

template <class hash_T>
unsigned int CoinQMerkleNode<hash_T>::getNumChildren() const
{
    if (!pLeftChild && !pRightChild) return 0;
    if (!pRightChild) return 1;
    assert(pLeftChild);
    return 2;
}

template <class hash_T>
bool CoinQMerkleNode<hash_T>::insertLeaf(CoinQMerkleNode<hash_T>* pLeaf, int insertDepth)
{
    assert(pLeaf);

    if (!pLeftChild && !pRightChild && !hash.empty()) return false; // we're already at a leaf node.

    if (insertDepth <= 0) {
        if (!pLeftChild) {
            pLeftChild = pLeaf;
        }
        else if (!pRightChild) {
            pRightChild = pLeaf;
        }
        else if (insertDepth == 0) {
            return false;
        }
        else {
            // balance the tree
            unsigned int leftDepth = pLeftChild->getDepth();
            unsigned int rightDepth = pRightChild->getDepth();
            if (rightDepth < leftDepth && !pRightChild->insertLeaf(pLeaf, leftDepth) && !pLeftChild->insertLeaf(pLeaf, -1)) return false;
            if (leftDepth < rightDepth && !pLeftChild->insertLeaf(pLeaf, rightDepth) && !pRightChild->insertLeaf(pLeaf, -1)) return false;
            if (!pLeftChild->insertLeaf(pLeaf, -1) && !pRightChild->insertLeaf(pLeaf, -1)) return false;
        }
    }
    else {
        if (!pLeftChild || !pRightChild) {
            for (int i = 0; i < insertDepth; i++) {
                CoinQMerkleNode<hash_T>* pChild = pLeaf;
                pLeaf = new CoinQMerkleNode<hash_T>;
                pLeaf->pLeftChild = pChild;
            }

            if (!pLeftChild) {
                pLeftChild = pLeaf;
            }
            else {
                pRightChild = pLeaf;
            }
        }
        if (!pLeftChild->insertLeaf(pLeaf, insertDepth - 1) && !pRightChild->insertLeaf(pLeaf, insertDepth - 1)) return false;
    }

    bDepthChanged = true;
    return true;
}

template <class hash_T>
std::string CoinQMerkleNode<hash_T>::toString(const std::string& prefix) const
{
    std::stringstream ss;
    ss << prefix << " " << getHash().toString() << std::endl;

    if (pLeftChild) {
        ss << pLeftChild->toString(prefix + "l");
    }

    if (pRightChild) {
        ss << pRightChild->toString(prefix + "r");
    }
    return ss.str();
}


template <class hash_T>
class CoinQMerkleTree
{
private:
    CoinQMerkleNode<hash_T>* pRoot;

public:
    CoinQMerkleTree() : pRoot(NULL) { }
    ~CoinQMerkleTree() { if (pRoot) delete pRoot; }

    bool insertLeaf(CoinQMerkleNode<hash_T>* pLeaf, int insertDepth = -1);
    const CoinQMerkleNode<hash_T>& getRoot() const { assert (pRoot); return *pRoot; }

    bool isEmpty() const { return (pRoot == NULL); }
    void clear() { if (pRoot) delete pRoot; pRoot = NULL; }

    std::string toString() const { if (!pRoot) return "empty"; return pRoot->toString(); }
};

template <class hash_T>
bool CoinQMerkleTree<hash_T>::insertLeaf(CoinQMerkleNode<hash_T>* pLeaf, int insertDepth)
{
    assert(pLeaf);

    if (!pRoot) {
        if (insertDepth > 0) {
            pRoot = new CoinQMerkleNode<hash_T>;
            return pRoot->insertLeaf(pLeaf, insertDepth - 1);
        }
        else {
            pRoot = pLeaf;
            return true;
        }
    }

    if (!pRoot->insertLeaf(pLeaf, insertDepth)) {
        if (insertDepth == -1) {
            // Create new root, set left branch to old tree, and create a right branch to balance
            CoinQMerkleNode<hash_T>* pNewRoot = new CoinQMerkleNode<hash_T>;
            if (!pNewRoot->insertLeaf(pLeaf, 0)) return false;
            pRoot = pNewRoot;
            return pRoot->insertLeaf(pLeaf, -1);
        }
        return false; // Don't mess with tree depth if insertDepth was specified explicitly. Fail instead.
    }

    return true;
}

#endif // _COINQ_MERKLE_H_
