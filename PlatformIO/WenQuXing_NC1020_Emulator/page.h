#ifndef PAGE_H_
#define PAGE_H_

class Page {
  public:
    virtual void initPage() {}
    virtual void onDraw() = 0;
    virtual bool handleKey(const char * key) {
      return false;
    }
};

#endif /* PAGE_H_ */
