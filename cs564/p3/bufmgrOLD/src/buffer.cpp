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

    /**@brief
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



    /******************IMPLEMENTED FOR PROJECT 3, DYLAN FIEDLER AND MEL LAKRITZ**************/

    /**@brief
     * Advance clock to next frame in the buffer pool in a circular manner
     * using modulos
     */
    void BufMgr::advanceClock()
    {
        clockHand = (clockHand + 1) % numBufs;
    }

    /**@brief
     * Allocates a free frame using the clock algorithm; 
     * if necessary, writing a dirty page back to disk. Throws 
     * BufferExceededException if all buffer frames are pinned. 
     * This private method will get called by the readPage() and allocPage() 
     * methods described below. Make sure that if the buffer frame allocated
     * has a valid page in it, you remove the appropriate entry from 
     * the hash table.
     */
    void BufMgr::allocBuf(FrameId & frame) 
    {
        std::uint32_t count = 0;
        while (count <= numBufs) { //keep equal to?
            count++;
            advanceClock();

            BufDesc *buffer = bufDescTable + clockHand;

            if (buffer->valid) {
                //if pin pages, skip
                if (buffer->pinCnt > 0) {
                    continue;
                } 

                //reset refbit, skip
                if (buffer->refbit) {
                    buffer->refbit = false;
                    bufStats.accesses ++;
                    continue;
                }

                //writing a dirty page back to disk and increment diskwrite
                if (buffer->dirty) {
                    buffer->file->writePage(bufPool[clockHand]);
                    bufStats.diskwrites++;
                }

                //remove from hadshtable
                hashTable->remove(buffer->file, buffer->pageNo);
            }

             //move rame to new clockhand
            frame = clockHand;
            //clear previous desc
            buffer->Clear();

            //remember to return, otherwise fails test1 1
           return;
        }

        //throw exception of all buffer frames are pinned
        throw BufferExceededException();
    }

    /**@brief
     * Case 1: Check whether the page is already in the buffer pool, if so
     * set the appropriate refbit, increment the pinCnt for the page, and
     * then return a pointer to the frame containing the page via the page 
     * parameter.
     *
     * Case 2:  Call allocBuf() to allocate a buffer frame and then call the 
     * method file->readPage() to read the page from disk into the buffer pool
     * frame. Next, insert the page into the hashtable. Finally, invoke Set()
     * on the frame to set it up properly. Set() will leave the pinCnt for the 
     * page set to 1. Return a pointer to the frame containing the page via 
     * the page parameter
     */
    void BufMgr::readPage(File* file, const PageId pageNo, Page*& page)
    {
        try {
            //Case 1: in buffer, set refbit, increment pinCnt, return ptr to page
            FrameId frameNo;
            hashTable->lookup(file, pageNo, frameNo);
            bufDescTable[frameNo].refbit = true;
            bufDescTable[frameNo].pinCnt++;
            //to return ptr to page
            //is it actually supposed to return or just change the value of page ptr???
            page = bufPool + frameNo;
        } catch (HashNotFoundException hashNotFound) {
            //Case 2: call alocbuf(), readpage into buffer pool, set frame, return ptr
            FrameId frameNo;
            allocBuf(frameNo);

            //add disk reads
            bufStats.diskreads++;
            bufPool[frameNo] = file->readPage(pageNo);

            //insert into has table
            hashTable->insert(file, pageNo, frameNo);
            bufDescTable[frameNo].Set(file, pageNo);

            //add to page
            page = bufPool + frameNo;
        }
    }

    /**@brief
     * Decrements the pinCnt of the frame containing (file, PageNo) and, 
     * if dirty == true, sets the dirty bit. Throws PAGENOTPINNED if the pin 
     * count is already 0. Does nothing if page is not found in the hash table 
     * lookup.
     */
    void BufMgr::unPinPage(File* file, const PageId pageNo, const bool dirty) 
    {
        try {
            FrameId frameNo;
            hashTable->lookup(file, pageNo, frameNo);

             //throw PAGENOTPINNEDEXCEPTION if pinCnt is already 0
            if (bufDescTable[frameNo].pinCnt == 0){
                throw PageNotPinnedException(file->filename(), pageNo, frameNo);
            } else {
              //reduce pincnt and set dirty bit if true
              if (dirty == true){
                 bufDescTable[frameNo].dirty = true;
             }

             bufDescTable[frameNo].pinCnt--;
           }
        } catch (HashNotFoundException hashNotFound) {
            // do nothing when page not in hashtable
        }
    }

    /**@brief
     * Scan bufTable for pages belonging to the file. For each page encountered 
     * it should: (a) if the page is dirty, call file->writePage() to flush the page 
     * to disk and then set the dirty bit for the page to false, (b) remove the page
     * from the hashtable (whether the page is clean or dirty) and (c) invoke the 
     * Clear() method of BufDesc for the page frame. Throws PagePinnedException if 
     * some page of the file is pinned. Throws BadBufferException if an invalid page
     * belonging to the file is encountered.
     */
    void BufMgr::flushFile(const File* file) 
    {
        //Scan bufTable for pages belonging to the file
        FrameId i;
        for (i = 0; i < numBufs; i++) {

            BufDesc *buffer = bufDescTable + i;
            if (buffer->file == file) {

                //Throws PagePinnedException if at least one file is pinned
                if (buffer->pinCnt != 0){
                    throw PagePinnedException(file->filename(), buffer->pageNo, i);
                }

                //Throws BadBufferException if an invalid page belonging to the file is encountered.
                if (!buffer->valid) {
                    throw BadBufferException(i, buffer->dirty, buffer->valid, buffer->refbit);
                }

                //(a) if dirty, call file-writePage() to flush the page to disk and set dirty bit to false
                if (buffer->dirty) {
                    buffer->dirty = false;
                    buffer->file->writePage(bufPool[i]);   
                }
                
                //(b) always remove the page from hashtable
                hashTable->remove(file, buffer->pageNo);

                //(c) throws clear method of BufDesc for the page frame
                buffer->Clear();
            }
        }
    }

    /**@brief
     * Allocate an empty page in the specified file by invoking the file->allocatePage() 
     * method. This method will return a newly allocated page. Then allocBuf() is called 
     * to obtain a buffer pool frame. Next, an entry is inserted into the
     * hash table and Set() is invoked on the frame to set it up properly. The method 
     * returns both the page number of the newly allocated page to the caller via the 
     * pageNo parameter and a pointer to the buffer frame allocated for the page via 
     * the page parameter.
     */

    void BufMgr::allocPage(File* file, PageId &pageNo, Page*& page) 
    {
        // allocate an empty page and get its a page number
        Page emptyPage = file->allocatePage();
        pageNo = emptyPage.page_number();

        //allocBuff to obtain buffer pool frame
        FrameId frameNo;
        allocBuf(frameNo);
       
        //insert into has table
        hashTable->insert(file, pageNo, frameNo);
       
        //set to setup up frame properly
        bufDescTable[frameNo].Set(file, pageNo);
        bufPool[frameNo] = emptyPage;
        page = bufPool + frameNo;
    }

    /**@brief
     * This method deletes a particular page from file. Before deleting the page
     * from file, it sure that if the page to be deleted is allocated a frame in
     * the buffer pool, that frame is freed and correspondingly entry from hash 
     * table is also removed.
     */
    void BufMgr::disposePage(File* file, const PageId PageNo)
    {
        try {
            FrameId frameNo;
            hashTable->lookup(file, PageNo, frameNo);
            //free frame in buffer pool and hash table before deleting the file
            bufDescTable[frameNo].Clear();
            hashTable->remove(file, PageNo);
        } catch(HashNotFoundException hashNotFound) {
          //do nothing if page is not found
        }

        // actually delete the page
        file->deletePage(PageNo);
    }

    /**************************END OF IMPLEMENTATION ***************************/

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
