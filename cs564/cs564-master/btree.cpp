/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include "btree.h"
#include "filescan.h"
#include "exceptions/bad_index_info_exception.h"
#include "exceptions/bad_opcodes_exception.h"
#include "exceptions/bad_scanrange_exception.h"
#include "exceptions/no_such_key_found_exception.h"
#include "exceptions/scan_not_initialized_exception.h"
#include "exceptions/index_scan_completed_exception.h"
#include "exceptions/file_not_found_exception.h"
#include "exceptions/end_of_file_exception.h"
#include "exceptions/file_exists_exception.h"
#include "exceptions/badgerdb_exception.h"
#include "exceptions/page_not_pinned_exception.h"

//#define DEBUG

namespace badgerdb
{

    // -----------------------------------------------------------------------------
    // BTreeIndex::BTreeIndex -- Constructor
    // -----------------------------------------------------------------------------

    BTreeIndex::BTreeIndex(const std::string & relationName,
            std::string & outIndexName,
            BufMgr *bufMgrIn,
            const int attrByteOffset,
            const Datatype attrType)
    {
        // set instance variables
        this->bufMgr = bufMgrIn;
        this->attrByteOffset = attrByteOffset;
        this->attributeType = attrType;
        switch (attrType) {
            case INTEGER:
                this->leafOccupancy = INTARRAYLEAFSIZE;
                this->nodeOccupancy = INTARRAYNONLEAFSIZE;
                break;
            case DOUBLE:
                this->leafOccupancy = DOUBLEARRAYLEAFSIZE;
                this->nodeOccupancy = DOUBLEARRAYNONLEAFSIZE;
                break;
            case STRING:
                this->leafOccupancy = STRINGARRAYLEAFSIZE;
                this->nodeOccupancy = STRINGARRAYNONLEAFSIZE;
                break;
        }

        // set output index file name
        std::ostringstream idxStr;
        idxStr << relationName << "." << attrByteOffset;
        outIndexName = idxStr.str();

        try {
            // create new BlobFile
            this->file = new BlobFile(outIndexName, true);

            // set up index meta data info page (fist page of btree index file)
            // and get a root page 
            Page *metaPage;
            Page *rootPage;
            // meta page number: 1
            bufMgr->allocPage(file, headerPageNum, metaPage);
            // root page number: 2
            // only 2 when it is a leaf
            // when split rootpagenum changes
            bufMgr->allocPage(file, rootPageNum, rootPage);
            IndexMetaInfo *metaInfo = (IndexMetaInfo *)metaPage;

            // set up meta info
            relationName.copy(metaInfo->relationName, 20, 0); // relation name no longer than 20
            metaInfo->attrByteOffset = attrByteOffset;
            metaInfo->attrType = attrType;
            metaInfo->rootPageNo = rootPageNum;

            bufMgr->unPinPage(file, headerPageNum, true);
            bufMgr->unPinPage(file, rootPageNum, true);
            bufMgr->flushFile(file);

            // scan the relation and 
            // insert entries for all the tuples in this relation into the index
            FileScan relationScan(relationName, bufMgr);
            RecordId recordId;
            while (true) {
                relationScan.scanNext(recordId);
                const char *record = relationScan.getRecord().c_str();
                if (attributeType == STRING) {
                    char *keyptr = (char *)(record + attrByteOffset);
                    char keyarr[STRINGSIZE];
                    strncpy(keyarr, keyptr, sizeof(keyarr));
                    std::string key = std::string(keyarr);
                    insertEntry(&key, recordId);
                } else if (attributeType == DOUBLE) {
                    double *key = (double *)(record + attrByteOffset);
                    insertEntry(static_cast<void *>(key), recordId);
                } else if (attributeType == INTEGER) {
                    int *key = (int *)(record + attrByteOffset);
                    insertEntry(static_cast<void *>(key), recordId);
                }
            }

        } catch (FileExistsException fee) {
            // if exists, read the existed file
            this->file = new BlobFile(outIndexName, false);
            headerPageNum = 1; // meta page should always be the first page of index file
            Page *metaPage;
            bufMgr->readPage(file, headerPageNum, metaPage);
            IndexMetaInfo *metaInfo = (IndexMetaInfo *) metaPage;
            rootPageNum = metaInfo->rootPageNo;

            if (metaInfo->attrType != attrType ||
                    relationName.compare(metaInfo->relationName) != 0 ||
                    metaInfo->attrByteOffset != attrByteOffset) {
                throw BadIndexInfoException("constructor parameters do not match existed index file!");
            }
            bufMgr->unPinPage(file, headerPageNum, false);

        } catch (EndOfFileException eofe) {
            // insertion completed
        }

        this->scanExecuting = false;
    }


    // -----------------------------------------------------------------------------
    // BTreeIndex::~BTreeIndex -- destructor
    // -----------------------------------------------------------------------------

    BTreeIndex::~BTreeIndex() 
    {
        try {
            // unpin pages, clear variables
            endScan();
            bufMgr->flushFile(file);
        } catch (BadgerDbException bde) {
            // should not throw any exceptions
        }
        delete file;
    }

    // -----------------------------------------------------------------------------
    // BTreeIndex::assignKey
    // -----------------------------------------------------------------------------
    void BTreeIndex::assignKey(void *nodeptr, int pos, const void *key, bool isNL, bool clear) {
        if (isNL) {
            if (attributeType == INTEGER) {
                NonLeafNodeInt *ptr = (NonLeafNodeInt *)nodeptr;
                if (clear) ptr->keyArray[pos] = 0;
                else ptr->keyArray[pos] = *((int *)key);
            } else if (attributeType == DOUBLE) {
                NonLeafNodeDouble *ptr = (NonLeafNodeDouble *)nodeptr;
                if (clear) ptr->keyArray[pos] = 0.0;
                else ptr->keyArray[pos] = *((double *)key);
            } else if (attributeType == STRING) {
                NonLeafNodeString *ptr = (NonLeafNodeString *)nodeptr;
                if (clear) memset(ptr->keyArray + pos, '\0', sizeof(char) * STRINGSIZE);
                else strncpy(ptr->keyArray[pos], ((std::string *)key)->c_str(), STRINGSIZE);
            }
        } else {
            if (attributeType == INTEGER) {
                LeafNodeInt *ptr = (LeafNodeInt *)nodeptr;
                if (clear) ptr->keyArray[pos] = 0;
                else ptr->keyArray[pos] = *((int *)key);
            } else if (attributeType == DOUBLE) {
                LeafNodeDouble *ptr = (LeafNodeDouble *)nodeptr;
                if (clear) ptr->keyArray[pos] = 0.0;
                else ptr->keyArray[pos] = *((double *)key);
            } else if (attributeType == STRING) {
                LeafNodeString *ptr = (LeafNodeString *)nodeptr;
                if (clear) memset(ptr->keyArray + pos, '\0', sizeof(char) * STRINGSIZE);
                else strncpy(ptr->keyArray[pos], ((std::string *)key)->c_str(), STRINGSIZE);
            }
        }
    }


    // -----------------------------------------------------------------------------
    // BTreeIndex::searchLeaf
    // -----------------------------------------------------------------------------
    template<typename KT, typename NLT, typename LT, int NLS> 
        PageId BTreeIndex::searchLeaf(const KT *key, PageId *pidpath) 
        {
            if (rootPageNum == 2) // root is leaf
                return rootPageNum;
            Page *pageptr;
            PageId pagenum = rootPageNum;
            bufMgr->readPage(file, rootPageNum, pageptr);
            //RecordId recordId;
            int j = 0;
            while (true) {
                NLT *node = (NLT *)pageptr;
                pidpath[j] = pagenum;
                j++;
                int i;
                for (i = 0;node->pageNoArray[i+1] != Page::INVALID_NUMBER && compareKey(node->keyArray + i, key) <= 0 && i < NLS ; i++) {
                }
                bufMgr->unPinPage(file, pagenum, false);
                pagenum = node->pageNoArray[i];
                bufMgr->readPage(file, node->pageNoArray[i], pageptr);
                if (node->level == 1) {
                    bufMgr->unPinPage(file, pagenum, false);
                    return pagenum;
                }
            }
        }

    // -----------------------------------------------------------------------------
    // BTreeIndex::splitNonLeaf
    // -----------------------------------------------------------------------------
    template<typename KT, typename NLT, typename LT, int NLS>
        KT BTreeIndex::splitNonLeaf(PageId target, const KT key, PageId& newPid) {
            Page *targetPage;
            Page *newPage;
            bufMgr->readPage(file, target, targetPage);
            bufMgr->allocPage(file, newPid, newPage);

            NLT *targetNode = (NLT *)targetPage;
            NLT *newNode = (NLT *)newPage;

            newNode->level = targetNode->level;
            int index;
            for (index = 0; compareKey(targetNode->keyArray + index, &key) <=  0 && index < NLS; index ++);
            KT key_arr[NLS + 1];
            PageId pid_arr[NLS + 2];
            copyKeys(targetNode->keyArray, 0, index, key_arr, true);//std::copy(targetNode->keyArray, targetNode->keyArray + index, key_arr);
            std::copy(targetNode->pageNoArray,targetNode->pageNoArray+index+1,pid_arr);
            key_arr[index] = key;
            pid_arr[index+1] = 0;
            copyKeys(targetNode->keyArray, index, NLS, key_arr+index+1, true);//std::copy(targetNode->keyArray+index, targetNode->keyArray + NLS, key_arr + index + 1);
            std::copy(targetNode->pageNoArray+index+1,targetNode->pageNoArray+NLS+1,pid_arr+index+2);
            copyKeys(key_arr, 0, (NLS+1)/2, targetNode->keyArray);//std::copy(key_arr, key_arr + (NLS+1)/2, targetNode->keyArray);
            std::copy(pid_arr,pid_arr+(NLS+1)/2+1, targetNode->pageNoArray);

            copyKeys(key_arr, (NLS+1)/2+1, NLS+1, newNode->keyArray);//std::copy(key_arr + (NLS+1) / 2 + 1, key_arr + NLS + 1, newNode->keyArray);
            std::copy(pid_arr+(NLS+1)/2+1,pid_arr+NLS+2, newNode->pageNoArray); 
            std::fill_n(targetNode->pageNoArray+(NLS+1)/2+1, (NLS+1)-(NLS+1)/2-1,0);

            std::cout << target << " " << targetNode->keyArray[0] << "->" << targetNode->keyArray[(NLS+1)/2-1] << " " << targetNode->keyArray[(NLS+1)/2] << std::endl;
            std::cout << newPid << " " << newNode->keyArray[0] << "->" << newNode->keyArray[(NLS+1)/2] << " " << newNode->keyArray[(NLS+1)/2] << std::endl;
            std::cout << std::endl;

            bufMgr->unPinPage(file, target, true);
            bufMgr->unPinPage(file, newPid, true);
            return key_arr[(NLS+1) / 2];
        }

    

    /*
    void BTreeIndex::getKey(void *arr, int index, void *ret) {
        if (attributeType == INTEGER) {
            *((int *)ret) =  ((int *)arr)[index];
        } else if (attributeType == DOUBLE) {
            *((double *)ret) = ((double *)arr)[index];
        } else if (attributeType == STRING) {
            int offset = sizeof(char) * STRINGSIZE;
            *((std::string *)ret) = std::string((char *)arr + index * offset, (char *)arr + (index+1) * offset);
        }
    }
    */


    // -----------------------------------------------------------------------------
    // BTreeIndex::splitLeaf
    // -----------------------------------------------------------------------------
    template<typename KT, typename NLT, typename LT, int LS>
        KT BTreeIndex::splitLeaf(PageId target, const KT *key, const RecordId rid, PageId& newPid) {


            Page *targetPage;
            Page *newPage;
            bufMgr->readPage(file, target, targetPage);
            bufMgr->allocPage(file, newPid, newPage);

            LT *targetNode = (LT *)targetPage;
            LT *newNode = (LT *)newPage;
            std::cout << "connect " << target << "(" << targetNode->rightSibPageNo << ") " << newPid << std::endl;
            std::cout << "connect " << newPid << " " << targetNode->rightSibPageNo << std::endl;
            newNode->rightSibPageNo = targetNode->rightSibPageNo;
            targetNode->rightSibPageNo = newPid;

            RecordId invalidRid;
            invalidRid.page_number = Page::INVALID_NUMBER;

            int index;
            for (index = 0; compareKey(targetNode->keyArray + index, key) <=  0 && index < LS; index ++);
            KT key_arr[LS + 1];
            RecordId rid_arr[LS + 1];
            
            copyKeys(targetNode->keyArray, 0, index, key_arr, true);//std::copy(targetNode->keyArray, targetNode->keyArray + index, key_arr);
            std::copy(targetNode->ridArray, targetNode->ridArray + index, rid_arr);
            

            key_arr[index] = *key;
            rid_arr[index] = rid;
            
            copyKeys(targetNode->keyArray, index, LS, key_arr+index+1, true);//std::copy(targetNode->keyArray+index, targetNode->keyArray + LS, key_arr + index + 1);
            std::copy(targetNode->ridArray+index, targetNode->ridArray + LS, rid_arr + index + 1);

            copyKeys(key_arr, 0, (LS+1)/2, targetNode->keyArray);//std::copy(key_arr, key_arr + (LS+1) / 2, targetNode->keyArray);
            std::copy(rid_arr, rid_arr + (LS+1) / 2, targetNode->ridArray);
            
            copyKeys(key_arr, (LS+1)/2, LS+1, newNode->keyArray);//std::copy(key_arr + (LS+1) / 2, key_arr + LS + 1, newNode->keyArray);
            std::copy(rid_arr + (LS+1) / 2, rid_arr + LS + 1, newNode->ridArray);

            /*
            if (attributeType == INTEGER || attributeType == DOUBLE) {
                std::fill_n(targetNode->keyArray + (LS+1) / 2, LS - (LS+1) / 2, (KT)0);
                //memset(targetNode->keyArray +(LS+1)/2, sizeof(KT) * (LS - (LS+1)/2), 0); 
            } else {
                memset(targetNode->keyArray +(LS+1)/2, sizeof(char) * (LS - (LS+1)/2) * STRINGSIZE, 0); 
            }
            */
            std::fill_n(targetNode->ridArray + (LS+1) / 2, LS - (LS+1) / 2, invalidRid);
            /*
            std::cout << target << " " << targetNode->keyArray[0] << "->" << targetNode->keyArray[(LS+1)/2-1] << " " << targetNode->keyArray[(LS+1)/2] << std::endl;
            std::cout << newPid << " " << newNode->keyArray[0] << "->" << newNode->keyArray[(LS+1)/2-1] << " " << newNode->keyArray[(LS+1)/2] << std::endl;
            std::cout << std::endl;
            */
            if (rootPageNum != 2) {
            Page *rootPage;
            bufMgr->readPage(file, rootPageNum, rootPage);
            NLT *rootNode = (NLT *)rootPage;
            std::cout << rootPageNum << ": ";
            printNode(rootNode, true);
            bufMgr->unPinPage(file, rootPageNum, false);
            }
            std::cout << target << ": ";
            printNode(targetNode, false);
            std::cout << newPid << ": ";
            printNode(newNode, false);
            std::cout << "upward: " << key_arr[(LS+1)/2] << std::endl;
            std::cout << std::endl;

            std::cout << target << "->" << targetNode->rightSibPageNo << std::endl;
            bufMgr->unPinPage(file, target, true);
            bufMgr->unPinPage(file, newPid, true);

            return key_arr[(LS+1) / 2];
        }

    // -----------------------------------------------------------------------------
    // BTreeIndex::insertNonLeaf
    // -----------------------------------------------------------------------------
    template<typename KT, typename NLT, typename LT, int NLS>
        int BTreeIndex::insertNonLeaf(const KT key, PageId *pidpath, int depth, PageId& attachPage) {
            int pos;
            Page *curPage;
            bufMgr->readPage(file, pidpath[0], curPage);
            NLT *curNode = (NLT *)curPage;
            if (curNode->pageNoArray[NLS] != Page::INVALID_NUMBER) { // split and propagate upwards
                PageId newPid;
                KT upward_key = splitNonLeaf<KT, NLT, LT, NLS>(pidpath[0], key, newPid); 
                attachPage = newPid;
                pos = 0;
                if (curNode->level == depth) { // if path[0] is root
                    // set new rootPageNum info
                    Page *newRootPage;
                    Page *metaPage;

                    bufMgr->allocPage(file, rootPageNum, newRootPage);
                    bufMgr->readPage(file, headerPageNum, metaPage);

                    IndexMetaInfo *metaInfo = (IndexMetaInfo *)metaPage;
                    metaInfo->rootPageNo = rootPageNum;
                    NLT* newRootNode = (NLT *)newRootPage;
                    newRootNode->level = curNode->level + 1;
                    assignKey(newRootNode, 0, &upward_key, true, false);
                    newRootNode->pageNoArray[0] = pidpath[0];
                    newRootNode->pageNoArray[1] = newPid;

                    bufMgr->unPinPage(file, rootPageNum, true);
                    bufMgr->unPinPage(file, headerPageNum, true);
                } else {
                    PageId toAttachPid;
                    int j = insertNonLeaf<KT, NLT, LT, NLS>(upward_key, pidpath + 1, depth, toAttachPid);
                    Page *prevPage;

                    bufMgr->readPage(file, toAttachPid, prevPage);

                    NLT *prevNode = (NLT *)prevPage;
                    prevNode->pageNoArray[j] = newPid;

                    bufMgr->unPinPage(file, toAttachPid, true);
                }
            } else { // directly insert
                attachPage = pidpath[0];
                int i = 0,index=0;
                for(i = 0; i<NLS&&curNode->pageNoArray[i+1]!=Page::INVALID_NUMBER;i++) {
                    if(compareKey(curNode->keyArray+i,&key)<=0) {
                        index += 1;
                    }
                }
                //std::cout << "len: " << i << "; " << "index: " << index << " for " << key << std::endl;
                KT key_arr[i+1];
                copyKeys(curNode->keyArray, 0, index, key_arr, true);//std::copy(curNode->keyArray, curNode->keyArray + index, key_arr);
                key_arr[index] = key;
                copyKeys(curNode->keyArray, index, i, key_arr+index+1, true);//std::copy(curNode->keyArray+index, curNode->keyArray + i, key_arr + index + 1);

                copyKeys(key_arr, 0, i+1, curNode->keyArray);//std::copy(key_arr, key_arr+i+1, curNode->keyArray);
                pos = index+1;
                PageId pid_arr[NLS+2];
                std::copy(curNode->pageNoArray,curNode->pageNoArray+pos,pid_arr);
                std::copy(curNode->pageNoArray+pos,curNode->pageNoArray+NLS+1,pid_arr+pos+1);
                std::copy(pid_arr+pos+1,pid_arr+NLS+2,curNode->pageNoArray+pos+1);
                //assignKey(curNode, (i <= 1) ? i : i-1, &key, true, false);
            }
            bufMgr->unPinPage(file, pidpath[0], true);
            return pos;
        }

    // -----------------------------------------------------------------------------
    // BTreeIndex::copyKeys
    // -----------------------------------------------------------------------------
    void BTreeIndex::copyKeys(void *src, int start, int end, void *des, bool reverse) {
        if (attributeType == INTEGER) {
            std::copy(((int *)src) + start, ((int *)src) + end, (int *)des);
        } else if (attributeType == DOUBLE) {
            std::copy(((double *)src) + start, ((double *)src) + end, (double *)des);
        } else if (attributeType == STRING) {
            if (!reverse) {
                for (int i = start, j = 0; i < end; i++, j++) {
                    strcpy(((char *)des)+j*STRINGSIZE*sizeof(char), (static_cast<std::string *>(src))[i].c_str());
                }
            } else {
                int offset = sizeof(char) * STRINGSIZE;
                for (int i = start, j = 0; i < end; i++, j++) {
                    int bias = i * offset;
                    (static_cast<std::string *>(des))[j] = std::string(((char *)src) + bias, ((char *)src) + bias + offset);
                }
            }
        }
    }

    // -----------------------------------------------------------------------------
    // BTreeIndex::insertLeaf
    // -----------------------------------------------------------------------------
    template<typename KT, typename NLT, typename LT, int LS>
        bool BTreeIndex::insertLeaf(PageId target, const KT *key, const RecordId rid) {
            Page *targetPage;
            bufMgr->readPage(file, target, targetPage);
            LT *targetNode = (LT *)targetPage;
            if (targetNode->ridArray[LS-1].page_number != Page::INVALID_NUMBER) {
                bufMgr->unPinPage(file, target, false);
                return false;
            }
            int i = 0,index=0;
            for(i = 0; i<LS&&targetNode->ridArray[i].page_number!=Page::INVALID_NUMBER;i++) {
                if(compareKey(targetNode->keyArray+i,key)<=0) {
                    index += 1;
                }
            }
            //std::cout << index << " " << i << std::endl;
            KT key_arr[i+1];
            RecordId rid_arr[i+1];
            copyKeys(targetNode, 0, index, key_arr, true);//std::copy(targetNode->keyArray, targetNode->keyArray + index, key_arr);
            std::copy(targetNode->ridArray, targetNode->ridArray + index, rid_arr);
            key_arr[index] = *key;
            rid_arr[index] = rid;
            copyKeys(targetNode->keyArray, index, i, key_arr+index+1, true);//std::copy(targetNode->keyArray+index, targetNode->keyArray + i, key_arr + index + 1);
            std::copy(targetNode->ridArray+index, targetNode->ridArray+i, rid_arr+index+1);

            copyKeys(key_arr, 0, i+1, targetNode->keyArray); //std::copy(key_arr, key_arr+i+1, targetNode->keyArray);
            std::copy(rid_arr, rid_arr+i+1, targetNode->ridArray);
            //assignKey(targetNode, i, key, false, false);
            //targetNode->ridArray[i] = rid;
            bufMgr->unPinPage(file, target, true);
            return true;
        }

    // -----------------------------------------------------------------------------
    // BTreeIndex::insertHelper
    // -----------------------------------------------------------------------------
    template<typename KT, typename NLT, typename LT, int LS, int NLS>
        void BTreeIndex::insertHelper(const KT *key, RecordId rid) {

            // when there is no root yet,
            // create root as a NLT and create first data page 
            // note: left subtree key < key <= right subtree key
            if (rootPageNum == Page::INVALID_NUMBER) {
                // this should not happen
                exit(1);
            } else if (rootPageNum == 2) { // root is leaf
                if (!insertLeaf<KT, NLT, LT, LS>(rootPageNum, key, rid)) {
                    PageId newPid;
                    KT upward_key = splitLeaf<KT, NLT, LT, LS>(rootPageNum, key, rid, newPid);

                    Page *newRootPage;
                    PageId oldPid = rootPageNum;
                    bufMgr->allocPage(file, rootPageNum, newRootPage);
                    NLT *newRootNode = (NLT *)newRootPage;
                    newRootNode->level = 1;
                    assignKey(newRootNode, 0, &upward_key, true, false);
                    newRootNode->pageNoArray[0] = oldPid;
                    newRootNode->pageNoArray[1] = newPid;
                    bufMgr->unPinPage(file, rootPageNum, true);

                    Page *metaPage;
                    bufMgr->allocPage(file, headerPageNum, metaPage);
                    IndexMetaInfo *metaInfo = (IndexMetaInfo *)metaPage;
                    metaInfo->rootPageNo = rootPageNum;
                    bufMgr->unPinPage(file, headerPageNum, true);
                }
            } else {
                int depth = 0; // length of path
                PageId *pidpath = NULL;
                Page *rootPage;
                bufMgr->readPage(file, rootPageNum, rootPage);
                NLT *rootNode = (NLT *)rootPage;
                pidpath = new PageId[rootNode->level];
                depth = rootNode->level;
                bufMgr->unPinPage(file, rootPageNum, false);
                PageId targetLeaf = searchLeaf<KT, NLT, LT, NLS>(key, pidpath);
                std::cout << (*key) << " " << targetLeaf << std::endl;
                if (!insertLeaf<KT, NLT, LT, LS>(targetLeaf, key, rid)) {
                    PageId newPid;
                    KT upward_key = splitLeaf<KT, NLT, LT, LS>(targetLeaf, key, rid, newPid);
                    PageId attachPid;
                    int j = insertNonLeaf<KT, NLT, LT, NLS>(upward_key, pidpath, depth,attachPid);
                    Page *attachPage;
                    bufMgr->readPage(file, attachPid, attachPage);
                    NLT *attachNode = (NLT *)attachPage;
                    attachNode->pageNoArray[j] = newPid;
                    bufMgr->unPinPage(file, attachPid, true);
                    delete pidpath;

                }
            }
        }

    // -----------------------------------------------------------------------------
    // BTreeIndex::insertEntry
    // -----------------------------------------------------------------------------

    const void BTreeIndex::insertEntry(const void *key, const RecordId rid) 
    {
        if (attributeType == INTEGER) {
            insertHelper<int, NonLeafNodeInt, LeafNodeInt, INTARRAYLEAFSIZE, INTARRAYNONLEAFSIZE>(static_cast<const int *>(key), rid);
        } else if (attributeType == DOUBLE) {
            insertHelper<double, NonLeafNodeDouble, LeafNodeDouble, DOUBLEARRAYLEAFSIZE, DOUBLEARRAYNONLEAFSIZE>(static_cast<const double *>(key), rid);
        } else if (attributeType == STRING) {
            insertHelper<std::string, NonLeafNodeString, LeafNodeString, STRINGARRAYLEAFSIZE, STRINGARRAYNONLEAFSIZE>(static_cast<const std::string *>(key), rid);
        }
    }

    // -----------------------------------------------------------------------------
    // BTreeIndex::startScan
    // -----------------------------------------------------------------------------

    int BTreeIndex::compareKey(const void *key1, const void *key2, bool isStr) {
        if (attributeType == INTEGER) {
            int k1 = *((int *)key1), k2 = *((int *)key2);
            if (k1 < k2) return -1;
            else if (k1 > k2) return 1;
            else return 0;
        } else if (attributeType == DOUBLE) {
            double k1 = *((double *)key1), k2 = *((double *)key2);
            if (k1 < k2) return -1;
            else if (k1 > k2) return 1;
            else return 0;
        } else if (attributeType == STRING) {
            std::string k1;
            k1 = std::string((char *)key1).substr(0, STRINGSIZE);
            std::string k2;
            if (isStr) k2= *((std::string *)key2);
            else k2 = std::string((char *)key2).substr(0, STRINGSIZE);
            if (k1 < k2) return -1;
            else if (k1 > k2) return 1;
            else return 0;
        }
        return 0;
    }


    // -----------------------------------------------------------------------------
    // BTreeIndex::scanLeaf
    // -----------------------------------------------------------------------------
    template<typename KT, typename NLT, typename LT, int LS, int NLS>
        void BTreeIndex::scanLeaf(const void *lowValParm,
                const Operator lowOpParm,
                const void *highValParm,
                const Operator highOpParm) {
            bufMgr->readPage(file, currentPageNum, currentPageData);
            LT *currentNode = (LT *)currentPageData;
            int i;
            for (i = 0; i < LS && compareKey(currentNode->keyArray + i, lowValParm, false) < 0; i++);
            if (lowOp == GT && compareKey(currentNode->keyArray + i, lowValParm, false) == 0) {
                i ++;
            }
            int flag = compareKey(currentNode->keyArray + i, highValParm, false);
            if (((highOp == badgerdb::Operator::LT) && (flag == 0)) || flag > 0 || i >= LS) {
                throw NoSuchKeyFoundException();
            }
            nextEntry = i;
            // do not unpin, since we want target page pinned in buffer pool
        }

    // -----------------------------------------------------------------------------
    // BTreeIndex::startScan
    // -----------------------------------------------------------------------------
    template<typename KT, typename NLT, typename LT, int LS, int NLS>
        void BTreeIndex::scanHelper(const void *lowValParm, 
                const Operator lowOpParm,
                const void *highValParm,
                const Operator highOpParm) {
            if (rootPageNum == Page::INVALID_NUMBER) { // invalid
                exit(1);
            } else if (rootPageNum == 2) { // root is leaf
                currentPageNum = rootPageNum;
                scanLeaf<KT, NLT, LT, LS, NLS>(lowValParm, lowOpParm, highValParm, highOpParm);
            } else {
                currentPageNum = rootPageNum;
                while (true) {
                    bufMgr->readPage(file, currentPageNum, currentPageData);
                    NLT *currentNode = (NLT *)currentPageData;
                    int i;
                    for (i = 0; currentNode->pageNoArray[i] != Page::INVALID_NUMBER && i < NLS && compareKey(currentNode->keyArray + i, lowValParm, false) < 0; i++);

                    if (lowOp == GT && compareKey(currentNode->keyArray + i, lowValParm, false) == 0) {
                        i ++;
                    }
                    PageId oldPageNum = currentPageNum;
                    if (currentNode->level == 1) {
                        currentPageNum = currentNode->pageNoArray[i];
                        bufMgr->unPinPage(file, oldPageNum, false);
                        std::cout << "find leaf: " << currentPageNum << std::endl;
                        break;
                    }
                    currentPageNum = currentNode->pageNoArray[i];
                    bufMgr->unPinPage(file, oldPageNum, false);
                }
                scanLeaf<KT, NLT, LT, LS, NLS>(lowValParm, lowOpParm, highValParm, highOpParm);
            }
        }

    // -----------------------------------------------------------------------------
    // BTreeIndex::startScan
    // -----------------------------------------------------------------------------

    const void BTreeIndex::startScan(const void* lowValParm,
            const Operator lowOpParm,
            const void* highValParm,
            const Operator highOpParm)
    {
        if (compareKey(lowValParm, highValParm, false) > 0) {
            throw BadScanrangeException();
        }
        if ((lowOpParm != GT && lowOpParm != GTE) ||
                (highOpParm != LT && highOpParm != LTE)) {
            throw BadOpcodesException();
        }
        if (scanExecuting == true) {
            endScan();
        } 
        scanExecuting = true;
        lowOp = lowOpParm;
        highOp = highOpParm;
        if (attributeType == INTEGER) {
            lowValInt = *((int *)lowValParm);
            highValInt = *((int *)highValParm);
            std::cout << lowValInt << " " << highValInt << std::endl;
            scanHelper<int, NonLeafNodeInt, LeafNodeInt, INTARRAYLEAFSIZE, INTARRAYNONLEAFSIZE>(lowValParm, lowOpParm, highValParm, highOpParm);
        } else if (attributeType == DOUBLE) {
            lowValDouble = *((double *)lowValParm);
            highValDouble = *((double *)highValParm);
            std::cout << lowValDouble << " " << highValDouble << std::endl;
            scanHelper<int, NonLeafNodeDouble, LeafNodeDouble, INTARRAYLEAFSIZE, INTARRAYNONLEAFSIZE>(lowValParm, lowOpParm, highValParm, highOpParm);
        } else if (attributeType == STRING) {
            lowValString = std::string((char *)lowValParm).substr(0, STRINGSIZE);
            highValString = std::string((char *)highValParm).substr(0, STRINGSIZE);
            std::cout << lowValString << " " << highValString << std::endl;
            scanHelper<int, NonLeafNodeString, LeafNodeString, INTARRAYLEAFSIZE, INTARRAYNONLEAFSIZE>(lowValParm, lowOpParm, highValParm, highOpParm);
        }
    }

    // -----------------------------------------------------------------------------
    // BTreeIndex::scanNext
    // -----------------------------------------------------------------------------

    const void BTreeIndex::scanNext(RecordId& outRid) 
    {
        if (!scanExecuting) {
            throw ScanNotInitializedException();
        }
        // maybe do not need this read, since this page is supposed to be pinned
        // in buffer manager?
        bufMgr->readPage(file, currentPageNum, currentPageData);
        if (attributeType == INTEGER) {
            LeafNodeInt *currentNode = (LeafNodeInt *)currentPageData;

            if (currentPageNum == Page::INVALID_NUMBER || currentNode->keyArray[nextEntry] > highValInt ||
                    (currentNode->keyArray[nextEntry] == highValInt && highOp == LT)) {
                throw IndexScanCompletedException();
            }

            outRid = currentNode->ridArray[nextEntry];
            nextEntry ++;
            if (nextEntry >= INTARRAYLEAFSIZE || currentNode->ridArray[nextEntry].page_number == Page::INVALID_NUMBER) { // jump to right sibling
                unPinCurrentPage();
                currentPageNum = currentNode->rightSibPageNo;
                nextEntry = 0;
                if (currentPageNum != Page::INVALID_NUMBER) {
                    bufMgr->readPage(file, currentPageNum, currentPageData);
                }
            }
        } else if (attributeType == DOUBLE) {
            LeafNodeDouble *currentNode = (LeafNodeDouble *)currentPageData;

            if (currentPageNum == Page::INVALID_NUMBER || currentNode->keyArray[nextEntry] > highValDouble ||
                    (currentNode->keyArray[nextEntry] == highValDouble && highOp == LT)) {
                throw IndexScanCompletedException();
            }

            outRid = currentNode->ridArray[nextEntry];
            nextEntry ++;
            if (nextEntry >= INTARRAYLEAFSIZE || currentNode->ridArray[nextEntry].page_number == Page::INVALID_NUMBER) { // jump to right sibling
                std::cout << "switch " << currentPageNum << "->" << currentNode->rightSibPageNo << std::endl;
                unPinCurrentPage();
                currentPageNum = currentNode->rightSibPageNo;
                nextEntry = 0;
                bufMgr->readPage(file, currentPageNum, currentPageData);
            }
        } else if (attributeType == STRING) {
            LeafNodeString *currentNode = (LeafNodeString *)currentPageData;

            if (currentPageNum == Page::INVALID_NUMBER || compareKey(currentNode->keyArray+nextEntry, &highValString) > 0 ||
                    (compareKey(currentNode->keyArray+nextEntry, &highValString)==0 && highOp == LT)) {
                throw IndexScanCompletedException();
            }
            std::cout << std::string(currentNode->keyArray[nextEntry]).substr(0,10) << std::endl;

            outRid = currentNode->ridArray[nextEntry];
            nextEntry ++;
            if (nextEntry >= STRINGARRAYLEAFSIZE || currentNode->ridArray[nextEntry].page_number == Page::INVALID_NUMBER) { // jump to right sibling
                unPinCurrentPage();
                currentPageNum = currentNode->rightSibPageNo;
                nextEntry = 0;
                bufMgr->readPage(file, currentPageNum, currentPageData);
            }
        }
    }

    void BTreeIndex::unPinCurrentPage() {
        try {
            while (1)
                bufMgr->unPinPage(file, currentPageNum, false);
        } catch (PageNotPinnedException pnpe) {
        }
    }

    // -----------------------------------------------------------------------------
    // BTreeIndex::endScan
    // -----------------------------------------------------------------------------
    //
    const void BTreeIndex::endScan() 
    {
        if (!scanExecuting) {
            throw ScanNotInitializedException();
        }
        scanExecuting = false;
        try {
            while(1)
                bufMgr->unPinPage(file, currentPageNum, false); // true or false?
        } catch (PageNotPinnedException pnpe) {
        }
    }

}
