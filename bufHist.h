#define F_HIST "bufHist.bh"

void resetBuffHist();
void initDB();
void killDB();
void getMaxBID();
void openDB();
void closeDB();
void addBuff(char* str);
void prvBuf(char* str);
void nxtBuf(char* str);
void clearBuffHist();
