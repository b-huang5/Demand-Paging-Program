// #Names: Mason Leonard, Brian Huang
// #RedIDs: 818030805, 822761804
// #Course: CS 480-01
// #Assignment 3: Progamming Portion

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <math.h>
#include <string.h>
#include <bitset>
#include <string.h>
#include <unordered_map>
#include <algorithm>
#include <climits>

#define MIN_NUMOFPROCESS 0
#define MAX_NUMOFBITS 28
#define MAX_32BITS 32
#define LRU_LIMIT 10

using namespace std;

#include "output_mode_helpers.h"
#include "tracereader.h"
#include "pagetable.h"

// helper function to mask the address and shift the address for only the necessary bits
int bitMask(int position, int length)
{
  // calculate mask in decimal.
  unsigned int mask = pow(2, length) - 1;
  // shift the mask to the correct position to add to array.
  mask = mask << (position - length);
  return mask;
}

int main(int argc, char **argv)
{
  if (argc > 2)
  {
    // initialize page table and level
    PAGETABLE *pageTable = new PAGETABLE();
    LEVEL *lv = new LEVEL();
    // initialize input variables, counters, and flags
    int idx;
    int option;
    int hit = 0;
    int miss = 0;
    unsigned int cacheHitCount = 0;
    int numOfBits = 0;
    int bytesUsed;
    int process;
    bool nFlag = false;
    bool oFlag = false;
    bool bitFlag = false;
    bool offsetFlag = false;
    bool vpn2pfnFlag = false;
    bool v2pFlag = false;
    bool tlbFlag = false;
    bool cacheFlag = false;
    bool summaryFlag = false;

    unordered_map<unsigned int, unsigned int> TLB;
    unordered_map<unsigned int, unsigned int> LRU;

    // increment optind to allow getopt to work.
    optind++;
    // grabs optional command arguments and sets flags based on what command arguements are inputted by the user.
    while ((option = getopt(argc, argv, "o:n:c:")) != -1)
    {
      switch (option)
      {
        // if -o is inputted set appropriate flags
      case 'o':
        oFlag = true;
        char *strCompare;
        strCompare = optarg;
        if (strcmp(strCompare, "bitmasks") == 0)
        {
          bitFlag = true;
        }
        else if (strcmp(strCompare, "offset") == 0)
        {
          offsetFlag = true;
        }
        else if (strcmp(strCompare, "vpn2pfn") == 0)
        {
          vpn2pfnFlag = true;
          pageTable->output_pfn = new unsigned int[pageTable->levelCount];
        }
        else if (strcmp(strCompare, "virtual2physical") == 0)
        {
          v2pFlag = true;
        }
        else if (strcmp(strCompare, "v2p_tlb_pt") == 0)
        {
          tlbFlag = true;
        }
        else if (strcmp(strCompare, "summary") == 0)
        {
          summaryFlag = true;
        }
        break;
      // if -n is inputted
      case 'n':
        // process is the max amount of addresses to be processed
        process = atoi(optarg);
        nFlag = true;
        break;
      // if -c is inputted set cache size as long as its greater than 0
      case 'c':
        cacheFlag = true;
        pageTable->cacheSize = atoi(optarg);
        if (pageTable->cacheSize <= 0)
        {
          std::cout << "Cache capacity must be a number, greater than or equal to 0" << '\n';
          exit(0);
        }
        break;
      default:
        std::cout << "default" << '\n';
        exit(1);
        break;
      }
    }

    // decrement optind in order to get correct position for idx to initialize arrays.
    optind--;
    idx = optind;
    // initialize arrays in pagetable and fill arrays with correct information.
    if (idx < argc)
    {
      // set max depth
      pageTable->levelCount = ((int)argc - idx - 1);
      // initialize page table arrays
      pageTable->bitMaskAry = new unsigned int[pageTable->levelCount];
      pageTable->shiftAry = new unsigned int[pageTable->levelCount];
      pageTable->entryCount = new unsigned int[pageTable->levelCount];
      // iterate through the user inputted bits and calculate information
      for (int i = 0; i < pageTable->levelCount; i++)
      {
        // if user inputted bits are less than 0 exit error
        if (atoi(argv[idx + 1]) == MIN_NUMOFPROCESS)
        {
          std::cout << "Level 1 page table must be at least 1 bit" << '\n';
          exit(3);
        }
        // grab the maximum number of bits the first mask can be
        int mask = MAX_32BITS - numOfBits;
        // shift the maximum number of bits the first mask can be to length of first level specified by user
        pageTable->bitMaskAry[i] = bitMask(mask, atoi(argv[idx + 1]));
        // make sure we don't subtract from 0
        if (i == 0)
        {
          pageTable->shiftAry[i] = MAX_32BITS - atoi(argv[idx + 1]);
        }
        else
        {
          pageTable->shiftAry[i] = pageTable->shiftAry[i - 1] - atoi(argv[idx + 1]);
        }
        // 2^user inputted bits per level
        pageTable->entryCount[i] = pow(2, atoi(argv[idx + 1]));
        bytesUsed += pageTable->entryCount[i];
        // increment the num of bits processed for calculations above
        numOfBits += atoi(argv[idx + 1]);
        // if the number of bits go over the maximum allowed bits of 28 exit error
        if (numOfBits > MAX_NUMOFBITS)
        {
          std::cout << "Too many bits used in page tables" << '\n';
          exit(4);
        }
        // increment to grab next bit level
        idx++;
      }
      // create the initial level to search or insert
      pageTable->levelRoot = createLevel(pageTable, pageTable->levelRoot, 0);
    }

    FILE *ifp;      /* trace file */
    p2AddrTr trace; /* traced address */

    /* attrace.addrt to open trace file */
    if ((ifp = fopen(argv[1], "rb")) == NULL)
    {
      fprintf(stderr, "cannot open %s for reading\n", argv[1]);
      exit(5);
    }
    // integer to keep track of how many addresses are processed already
    int count = 0;
    // while there is still another address continue reading the file
    while (!feof(ifp))
    {
      // to break the while loop if theres a specific amount of addresses specified
      if (nFlag)
      {
        if (count >= process)
        {
          break;
        }
      }
      /* get next address and process */
      if (NextAddress(ifp, &trace))
      {
        // cache flag
        if (cacheFlag)
        {
          // get virtual page number
          unsigned int vpn = trace.addr >> (MAX_32BITS - numOfBits);
          // mask offset shift
          unsigned int offsetMask = bitMask(MAX_32BITS - numOfBits, MAX_32BITS - numOfBits);
          // offset mask to get only the necessary bits
          offsetMask = trace.addr & offsetMask;
          // if TLB[vpn] has a value enter, signify's cache hit
          if (!(TLB.find(vpn) == TLB.end()))
          {
            // if LRU[vpn] doesn't exist in the LRU because of past deletes enter
            if (LRU.find(vpn) == LRU.end())
            {
              // iterate to find the smallest access time and delete it
              int smallestAccessTime = INT_MAX;
              unsigned int virtualKey;
              std::unordered_map<unsigned int, unsigned int>::iterator it = LRU.begin();
              while (it != LRU.end())
              {
                if (smallestAccessTime > it->second)
                {
                  smallestAccessTime = it->second;
                  virtualKey = it->first;
                }
                it++;
              }
              LRU.erase(virtualKey);
              // LRU[vpn] gets the new access time
              LRU[vpn] = count;
            }
            // if LRU[vpn] does have a value then just update LRU
            else
            {
              LRU[vpn] = count;
            }
            // Increment cache hit
            cacheHitCount++;
            // prepare the offset mask for printing
            offsetMask = offsetMask + ((TLB[vpn]) * pow(2, (MAX_32BITS - numOfBits)));
            report_v2pUsingTLB_PTwalk(trace.addr, offsetMask, true, true);
          }
          // if the cache misses
          else
          {
            // if its in the page table enter
            if (searchPageTable(pageTable, pageTable->levelRoot, trace.addr))
            {
              // if TLB reaches max size enter
              if (TLB.size() == pageTable->cacheSize)
              {
                // iterate to find the smallest access time and delete it from LRU and TLB
                int smallestAccessTime = INT_MAX;
                unsigned int virtualKey;
                std::unordered_map<unsigned int, unsigned int>::iterator it = LRU.begin();
                while (it != LRU.end())
                {
                  if (smallestAccessTime > it->second)
                  {
                    smallestAccessTime = it->second;
                    virtualKey = it->first;
                  }
                  it++;
                }
                LRU.erase(virtualKey);
                TLB.erase(virtualKey);
                // LRU and TLB get the new frame number and access time
                LRU[vpn] = count;
                TLB[vpn] = searchFrameNumber(pageTable, pageTable->levelRoot, trace.addr);
              }
              else
              {
                // if TLB is not full but LRU has reached its max size
                if (LRU.size() == LRU_LIMIT)
                {
                  // iterate to find the smallest access time and delete it
                  int smallestAccessTime = INT_MAX;
                  unsigned int virtualKey;
                  std::unordered_map<unsigned int, unsigned int>::iterator it = LRU.begin();
                  while (it != LRU.end())
                  {
                    if (smallestAccessTime > it->second)
                    {
                      smallestAccessTime = it->second;
                      virtualKey = it->first;
                    }
                    it++;
                  }
                  LRU.erase(virtualKey);
                  // LRU and TLB get the new frame number and access time
                  LRU[vpn] = count;
                  TLB[vpn] = searchFrameNumber(pageTable, pageTable->levelRoot, trace.addr);
                }
                // if TLB and LRU are not full just enter in access time and frame number
                else
                {
                  TLB[vpn] = searchFrameNumber(pageTable, pageTable->levelRoot, trace.addr);
                  LRU[vpn] = count;
                }
              }
              // prepare the offset for printing
              offsetMask = offsetMask + (TLB[vpn] * pow(2, (MAX_32BITS - numOfBits)));
              report_v2pUsingTLB_PTwalk(trace.addr, offsetMask, false, true);
              hit++;
            }
            // if not in the page table as well
            else
            {
              // insert the address
              pageInsert(pageTable, trace.addr, pageTable->frameCount);
              // if TLB is full enter
              if (TLB.size() == pageTable->cacheSize)
              {
                // iterate to find the smallest access time and delete it from LRU and TLB
                int smallestAccessTime = INT_MAX;
                unsigned int virtualKey;
                std::unordered_map<unsigned int, unsigned int>::iterator it = LRU.begin();
                while (it != LRU.end())
                {
                  if (smallestAccessTime > it->second)
                  {
                    smallestAccessTime = it->second;
                    virtualKey = it->first;
                  }
                  it++;
                }
                LRU.erase(virtualKey);
                TLB.erase(virtualKey);
                // LRU and TLB get the new frame number and access time
                LRU[vpn] = count;
                TLB[vpn] = searchFrameNumber(pageTable, pageTable->levelRoot, trace.addr);
              }
              else
              {
                // TLB is not full but LRU is full enter
                if (LRU.size() == LRU_LIMIT)
                {
                  // iterate to find the smallest access time and delete it from LRU
                  int smallestAccessTime = INT_MAX;
                  unsigned int virtualKey;
                  std::unordered_map<unsigned int, unsigned int>::iterator it = LRU.begin();
                  while (it != LRU.end())
                  {
                    if (smallestAccessTime > it->second)
                    {
                      smallestAccessTime = it->second;
                      virtualKey = it->first;
                    }
                    it++;
                  }
                  LRU.erase(virtualKey);
                  // LRU and TLB get the new frame number and access time
                  LRU[vpn] = count;
                  TLB[vpn] = searchFrameNumber(pageTable, pageTable->levelRoot, trace.addr);
                }
                else
                {
                  // if LRU and TLB are not full just add the access time and frame number
                  TLB[vpn] = searchFrameNumber(pageTable, pageTable->levelRoot, trace.addr);
                  LRU[vpn] = count;
                }
              }
              // prepare the offset mask for printing
              offsetMask = offsetMask + (TLB[vpn] * pow(2, (MAX_32BITS - numOfBits)));
              report_v2pUsingTLB_PTwalk(trace.addr, offsetMask, false, false);
            }
          }
          // increment count at the end to signify one address has been fully processed
          count++;
        }
        else
        {
          // search the page table for the address, if it finds it increment hit, else increment miss and insert the address
          if (searchPageTable(pageTable, pageTable->levelRoot, trace.addr))
          {
            hit++;
          }
          else
          {
            // insert the address
            pageInsert(pageTable, trace.addr, pageTable->frameCount);
            miss++;
          }
          // virtual2physical flag
          if (v2pFlag)
          {
            // create the offset mask
            unsigned int offsetMask = bitMask(MAX_32BITS - numOfBits, MAX_32BITS - numOfBits);
            // mask the address to get the offset
            offsetMask = trace.addr & offsetMask;
            // add the offset mask plus the frame number in order to get correct number for outputting
            offsetMask = offsetMask + (searchFrameNumber(pageTable, pageTable->levelRoot, trace.addr) * pow(2, (MAX_32BITS - numOfBits)));
            report_virtual2physical(trace.addr, offsetMask);
          }
          // vpn2pfn flag
          if (vpn2pfnFlag)
          {
            for (int i = 0; i < pageTable->levelCount; i++)
            {
              // create the offsetmask for pfn outputting
              unsigned int output_pfn_mask = trace.addr & pageTable->bitMaskAry[i];
              // shift the bits in order to get only the necessary bits
              output_pfn_mask = output_pfn_mask >> pageTable->shiftAry[i];
              // put it into the array for formatting later in print statement
              pageTable->output_pfn[i] = output_pfn_mask;
            }
            report_pagemap(pageTable->levelCount, pageTable->output_pfn, searchFrameNumber(pageTable, pageTable->levelRoot, trace.addr));
          }
          // offset flag
          if (offsetFlag)
          {
            // create the offset mask
            unsigned int offsetMask = bitMask(MAX_32BITS - numOfBits, MAX_32BITS - numOfBits);
            // mask the address with the offset
            offsetMask = trace.addr & offsetMask;
            printf("%08X\n", offsetMask);
          }
          // increment count at the end to signify one address has been fully processed
          count++;
        }
      }
    }
    // if nothing specified report the summary by default
    if (!(oFlag))
    {
      report_summary(pow(2, sizeof(pageTable)), cacheHitCount, hit, hit + miss, pageTable->frameCount, (sizeof(pageTable) + sizeof(lv) + bytesUsed));
    }
    // summary flag
    if (summaryFlag)
    {
      report_summary(pow(2, sizeof(pageTable)), cacheHitCount, hit, hit + miss, pageTable->frameCount, (sizeof(pageTable) + sizeof(lv) + bytesUsed));
    }
    // bit mask flag
    if (bitFlag)
    {
      report_bitmasks(pageTable->levelCount, pageTable->bitMaskAry);
    }

    /* clean up and return success */
    fclose(ifp);
    return (0);
  }
  else
  {
    std::cout << "Not enough arguements" << '\n';
    exit(6);
  }
}