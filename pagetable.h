// #Names: Mason Leonard, Brian Huang
// #RedIDs: 818030805, 822761804
// #Course: CS 480-01
// #Assignment 3: Progamming Portion

#ifndef pagetable
#define pagetable
struct PAGETABLE
{
    struct LEVEL *levelRoot;
    int levelCount;
    unsigned int *bitMaskAry;
    unsigned int *shiftAry;
    unsigned int *entryCount;
    unsigned int *output_pfn;
    int frameCount;
    int cacheSize;
};
#endif

#ifndef level
#define level
struct LEVEL
{
    struct PAGETABLE *pageTableRoot;
    int depth;
    struct LEVEL **nextLevelPtr;
    struct MAP *ary;
    bool isLeafNode;
};
#endif

#ifndef map
#define map
struct MAP
{
    unsigned int frameNumber;
    bool validFlag;
};
#endif

LEVEL *createLevel(struct PAGETABLE *page, struct LEVEL *lv, int depth);
void pageInsert(struct LEVEL *lv, unsigned int addr, int frame);
void pageInsert(struct PAGETABLE *page, unsigned int addr, int frame);
bool searchPageTable(struct PAGETABLE *page, struct LEVEL *lv, unsigned int addr);
unsigned int searchFrameNumber(struct PAGETABLE *page, struct LEVEL *lv, unsigned int addr);