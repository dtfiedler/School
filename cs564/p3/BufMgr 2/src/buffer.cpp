/**
 * @author See Contributors.txt for code contributors and overview of BadgerDB.
 *
 * @section LICENSE
 * Copyright (c) 2012 Database Group, Computer Sciences Department, University of Wisconsin-Madison.
 */

#include <memory>
#include <iostream>
#include "buffer.h"
#include "exceptions/buffer_exceeded_exception.h"
#include "exceptions/page_not_pinned_exception.h"
#include "exceptions/page_pinned_exception.h"
#include "exceptions/bad_buffer_exception.h"
#include "exceptions/hash_not_found_exception.h"

namespace badgerdb { 

    BufMgr::BufMgr(std::uint32_t bufs)
        : numBufs(bufs) {
            bufDescTable = new BufDesc[bufs];

            for (FrameId i = 0; i < bufs; i++) 
            {
                bufDescTable[i].frameNo = i;
                bufDescTable[i].valid = false;
            }

            bufPool = new Page[bufs];

            int htsize = ((((int) (bufs * 1.2))*2)/2)+1;
            hashTable = new BufHashTbl (htsize);  // allocate the buffer hash table

            clockHand = bufs - 1;
        }

    /**
     * flush valid and dirty pages
     * deallocate bufDescTable and bufPool
     */
    BufMgr::~BufMgr() {
        for (FrameId i = 0; i < numBufs; i++) {
            if (bufDescTable[i].dirty && bufDescTable[i].valid) {
                bufDescTable[i].file->writePage(bufPool[i]);
            }
        }
        delete [] bufDescTable;
        delete [] bufPool;
    }

    /**
     * move in a circular way
     */
    void BufMgr::advanceClock()
    {
        clockHand = (clockHand + 1) % numBufs;
    }

    /**
     * advance pointer, only nonvalid 
     * or valid and no refbit and no pinCnt
     * pages should be replaced
     * and remove all stuff of replaced page
     */
    void BufMgr::allocBuf(FrameId & frame) 
    {
        std::uint32_t num_scanned = 0;
        while (num_scanned < 2 * numBufs) { 
            advanceClock();
            num_scanned ++;
            BufDesc *bufDesc = bufDescTable + clockHand;
            if (bufDesc->valid) {
                if (bufDesc->refbit) { // reset refbit
                    bufStats.accesses ++;
                    bufDesc->refbit = false;
                    continue;
                } else if (bufDesc->pinCnt > 0) {
                    continue;
                }
                if (bufDesc->dirty) { // flush dirty pages
                    bufStats.diskwrites ++;
                    bufDesc->file->writePage(bufPool[clockHand]);
                }
                hashTable->remove(bufDesc->file, bufDesc->pageNo);
            }
            bufDesc->Clear();
            frame = clockHand;
            return;
        }
        throw BufferExceededException(); // cannot find unpinned pages
    }

    /**
     * if already in, get index, set proper bits
     * if not in, insert the page into buffer
     */
    void BufMgr::readPage(File* file, const PageId pageNo, Page*& page)
    {
        try {
            // first lookup hashtable, if exists, return data
            FrameId frameId;
            hashTable->lookup(file, pageNo, frameId);
            bufDescTable[frameId].refbit = true;
            bufDescTable[frameId].pinCnt ++;
            page = bufPool + frameId;
        } catch (HashNotFoundException hnfe) {
            // if not in buffer, perform disk reads, insert new data
            FrameId frameId;
            allocBuf(frameId);
            bufStats.diskreads ++;
            bufPool[frameId] = file->readPage(pageNo);
            hashTable->insert(file, pageNo, frameId);
            bufDescTable[frameId].Set(file, pageNo);
            page = bufPool + frameId;
        }
    }

    /**
     * decrease pinCnt when page in buffer
     */
    void BufMgr::unPinPage(File* file, const PageId pageNo, const bool dirty) 
    {
        try {
            FrameId frameId;
            hashTable->lookup(file, pageNo, frameId);
            if (bufDescTable[frameId].pinCnt == 0)
                throw PageNotPinnedException(file->filename(), pageNo, frameId);
            bufDescTable[frameId].pinCnt --;
            if (dirty == true) bufDescTable[frameId].dirty = true;
        } catch (HashNotFoundException hnfe) {
            // do nothing when page not in buffer
        }
    }

    /**
     * write page to disk if dirty, clean page info
     */
    void BufMgr::flushFile(const File* file) 
    {
        for (FrameId i = 0; i < numBufs; i++) {
            BufDesc *bufDesc = bufDescTable + i;
            if (bufDesc->file == file) {
                // check on valid, pinCnt and dirty bit
                if (!bufDesc->valid)
                    throw BadBufferException(i, bufDesc->dirty, bufDesc->valid, bufDesc->refbit);
                if (bufDesc->pinCnt > 0)
                    throw PagePinnedException(file->filename(), bufDesc->pageNo, i);
                if (bufDesc->dirty) {
                    bufDesc->file->writePage(bufPool[i]);
                    bufDesc->dirty = false;
                }
                hashTable->remove(file, bufDesc->pageNo);
                bufDesc->Clear();
            }
        }
    }

    /**
     * allocate a frame, then put the page into that frame
     */

    void BufMgr::allocPage(File* file, PageId &pageNo, Page*& page) 
    {
        // allocate in file and then put into buffer
        Page newpage = file->allocatePage();
        pageNo = newpage.page_number();
        FrameId frameId;
        allocBuf(frameId);
        hashTable->insert(file, pageNo, frameId);
        bufDescTable[frameId].Set(file, pageNo);
        bufPool[frameId] = newpage;
        page = bufPool + frameId;
    }

    /**
     * delete page if in buffer
     * and delete page in file
     */
    void BufMgr::disposePage(File* file, const PageId PageNo)
    {
        try {
            // modify buffer
            FrameId frameId;
            hashTable->lookup(file, PageNo, frameId);
            bufDescTable[frameId].Clear();
            hashTable->remove(file, PageNo);
        } catch ( HashNotFoundException hnfe) {
        }
        // modify file
        file->deletePage(PageNo);
    }

    void BufMgr::printSelf(void) 
    {
        BufDesc* tmpbuf;
        int validFrames = 0;

        for (std::uint32_t i = 0; i < numBufs; i++)
        {
            tmpbuf = &(bufDescTable[i]);
            std::cout << "FrameNo:" << i << " ";
            tmpbuf->Print();

            if (tmpbuf->valid == true)
                validFrames++;
        }

        std::cout << "Total Number of Valid Frames:" << validFrames << "\n";
    }

}
