#include "test.h"

int main(void){
    Thing* thing = NewThing();
    AddData(thing, 1);
    AddData(thing, 2);
    AddData(thing, 3);
    AddData(thing, 4);
    AddData(thing, 5);
    PrintContents(thing);
    return 0;
}