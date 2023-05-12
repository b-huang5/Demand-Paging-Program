// #Names: Mason Leonard, Brian Huang
// #RedIDs: 818030805, 822761804
// #Course: CS 480-01
// #Assignment 3: Progamming Portion

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "pagetable.h"

using namespace std;

//creates the initial level for the pagetable.
LEVEL *createLevel(struct PAGETABLE *page, struct LEVEL *lv, int d)
{
    //create an array of Levels for the page table.
    lv = (LEVEL *)calloc(1, sizeof(LEVEL));
    //assign pointer for page back to level.
    lv->pageTableRoot = page;
    //initialize the frame count to 0 to count.
    lv->depth = d;
    //if this is the last level, create an array of maps to set the validFlags to false for later checking.
    if((page->levelCount - 1) == lv->depth)
    {
        //to indicate we reached the last level
        lv->isLeafNode = true;
        //initialize an array of maps to set valid flags to check later
        lv->ary = (MAP *)calloc(page->entryCount[lv->depth], sizeof(MAP));
        //set all valid flags to false initially
        for(int i = 0; i < page->entryCount[lv->depth]; i++)
        {
            lv->ary[i].validFlag = false;
        }
    }
    //if not the last level, create an array of LEVELS pointers to point to next level.
    else
    {
        //to inddicate we are not on the last level
        lv->isLeafNode = false;
        //initialize an array of level pointers that point to the next level
        lv->nextLevelPtr = (LEVEL **)calloc(page->entryCount[lv->depth], sizeof(LEVEL *));
    }
    return lv;
}

void pageInsert(struct LEVEL *lv, unsigned int addr, int frame)
{
    //link the page table and level pointers together to be able to access information from the page table
    struct PAGETABLE *pageTable = lv->pageTableRoot;
    //mask the index
    unsigned int index = addr & pageTable->bitMaskAry[lv->depth];
    //shift for only the necssary bits
    index = index >> pageTable->shiftAry[lv->depth];
    //if its the last node
    if(lv->isLeafNode)
    {
        //set valid flag to true to indicate this address is occupied
        lv->ary[index].validFlag = true;
        //save the frame number for later caluclations
        lv->ary[index].frameNumber = frame;
        //increment the frame count
        pageTable->frameCount++;
    }
    //if its not the last node
    else 
    {
        //if there is no pointer to point to the next level create one
        if(lv->nextLevelPtr[index] == NULL)
        {
            //create a level and point that level to the initially NULL pointer
            lv->nextLevelPtr[index] = createLevel(lv->pageTableRoot, lv, lv->depth + 1);
        }
        //continue down the newly created level
        pageInsert(lv->nextLevelPtr[index], addr, frame);
    }
}

//helper function
void pageInsert(struct PAGETABLE *page, unsigned int addr, int frame)
{
    pageInsert(page->levelRoot, addr, page->frameCount);
}

//search the pagetable for the address. If it cannot find it, create a new level for it.
bool searchPageTable(struct PAGETABLE *page, struct LEVEL *lv, unsigned int addr)
{
    //mask the address
    unsigned int maskedAdr = addr & page->bitMaskAry[lv->depth];
    //shift the address for only the necessary bits
    maskedAdr = maskedAdr >> page->shiftAry[lv->depth];
    //if its the last level
    if(lv->isLeafNode)
    {
        //check to see if this "address" has been occupied. return true if yes or no if false
        if(lv->ary[maskedAdr].validFlag)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    //if its not the last level
    else
    {
        //if its not the last level and the pointer doesn't point to anything return false as nothing occupies this space
        if(lv->nextLevelPtr[maskedAdr] == NULL)
        {
            return false;
        }
        //recursively call in order to go down levels until the bottom
        return searchPageTable(page, lv->nextLevelPtr[maskedAdr], addr);
    }
}

//search the pagetable till the last level to retrieve frame number similar to searchPageTable function.
unsigned int searchFrameNumber(struct PAGETABLE *page, struct LEVEL *lv, unsigned int addr)
{
    //mask the address
    unsigned int maskedAdr = addr & page->bitMaskAry[lv->depth];
    //shift the address for only the necessary bits
    maskedAdr = maskedAdr >> page->shiftAry[lv->depth];
    //if its the last level
    if(lv->isLeafNode)
    {
        //grab the frame number and return
        return lv->ary[maskedAdr].frameNumber;
    }
    //recursively call in order to go down levels until the bottom
    return searchFrameNumber(page, lv->nextLevelPtr[maskedAdr], addr);
}