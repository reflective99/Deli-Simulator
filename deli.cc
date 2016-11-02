//#include <stdlib.h>
#include <iostream>
#include "thread.h"
#include <assert.h>
#include <string>
#include <fstream>
#include <vector>
#include <stdlib.h>
using namespace std;

//shared state: board of orders, number of living cashier threads
//create specified number of threads for cashiers, create one thread for sandwich maker
//cashiers take orders, give sandwiches in FIFO order
//maker makes sandwiches closest to current sandwich number (initialized to -1)
//maker only makes sandwich when board is as full as possible (number of living cashier threads)

static int numCashierThreads = 0;
static int curNum = 0;

class Note {
  public:
    int cashierNum;
    int sandwichNum;
    Note(int c, int s) {
      cashierNum = c;
      sandwichNum = s;
    }
    Note() {
      cashierNum = -1;
      sandwichNum = -1;
    }
};

class InputData {
	public:
		int boardSize;
		vector<vector<int>* >* cashierLists;
		InputData(int b, vector<vector<int>* >* v){
			boardSize = b;
			cashierLists = v;
		}
};

class BulletinBoard{
  public:
  int mySize;
  vector<Note>* myNotes; //hardcode

    BulletinBoard(int size){
      myNotes = new vector<Note>(size);
      mySize = size;
      curNum = 0;

			for(int i = 0; i < mySize; i++){
        (*myNotes)[i] = Note(-1,-1);
      }
    }

    void post(int cashier_num, int sandwich_num){
      if(cashierOnBoard(cashier_num)||!hasSpace()){
        return;
      }
      for(int i = 0; i < mySize; i++){
        if((*myNotes)[i].cashierNum == -1) {

          (*myNotes)[i].cashierNum = cashier_num;
          (*myNotes)[i].sandwichNum = sandwich_num;
          curNum++;
          return;

        }
      }
    }

    Note pull(int lastMadeSandwich){
      int min = 100000;
      Note* closestNote;
      int closestIndex;
      for(int i = 0; i < mySize; i++){
        if(((*myNotes)[i]).cashierNum == -1){ continue; }
        int sand = (*myNotes)[i].sandwichNum;
        int dist = sand -lastMadeSandwich;
        if(dist < 0){
          dist *= -1;
        }
        if(dist < min){
          closestNote = &(*myNotes)[i];
          min = dist;
					closestIndex = i;
        }
      }
      curNum--;
      return *closestNote;
    }

    void removeNote(int cashier_num){
			for(int i = 0; i < mySize; i++){
        if((*myNotes)[i].cashierNum == cashier_num){
          (*myNotes)[i] = Note(-1,-1);
        }
      }
    }

    bool cashierOnBoard(int cashier_num){
      for(int i = 0; i < mySize; i++){
        if((*myNotes)[i].cashierNum == cashier_num){
          return true;
        }
      }
      return false;
    }

    bool hasSpace(){
      return curNum < mySize;
    }

    bool isEmpty(){
      return curNum == 0;
    }

    bool isSaturated(){
      int max;
      if(numCashierThreads < mySize) {
        max = numCashierThreads;
      } else {
 				max = mySize;
      }
      return curNum >= max;
    }
};


class Cashier{
	public:
		int myNum;
		int cur_sandwich_index;
		vector<int>* sandwiches;
		BulletinBoard* board;

    Cashier(int id, vector<int>* list, BulletinBoard* b){
      cur_sandwich_index = 0;
      myNum = id;
      board = b;
			sandwiches = list;
			numCashierThreads++;
    }

    Note nextNote() {
      int sandNum = (*sandwiches)[cur_sandwich_index++];
      return Note(myNum, sandNum);
    }

    bool hasNextNote(){
      return cur_sandwich_index != sandwiches->size();
    }

    BulletinBoard* getBoard(){
			return board;
    }
};


void debug(int cashier_num, vector<Note>* myNotes){
	cout << endl;
	cout << "Cashier " << cashier_num << ": NumActiveCashiers: " << numCashierThreads << endl;
	for(int i = 0; i < (*myNotes).size(); i++) {
    cout << "Note"<<i<<" "<<(*myNotes)[i].cashierNum<<", "<<(*myNotes)[i].sandwichNum << endl;
  }
}

void cashier_thread(void* arg){
  thread_lock(0);
  Cashier c = *((Cashier*)(arg));
  BulletinBoard* boardptr = c.getBoard();
  BulletinBoard board = *boardptr;
  while(c.hasNextNote()){

		while(board.cashierOnBoard(c.myNum) || !board.hasSpace()){
      //cout << "Sleeping ";
			//debug(c.myNum, board.myNotes);
			thread_wait(0,0);
      //cout << "Awoken ";
			//debug(c.myNum, board.myNotes);
 		}

    Note noteToPost = c.nextNote();
    int sand = noteToPost.sandwichNum;
	  board.post(c.myNum,sand);
		cout << "POSTED: cashier " << c.myNum << " sandwich " << sand << endl;
    thread_broadcast(0,0);

  }

  while(board.cashierOnBoard(c.myNum)){
		  //cout << "Sleeping ";
			//debug(c.myNum, board.myNotes);
			thread_wait(0,0);
      //cout << "Awoken ";
			//debug(c.myNum, board.myNotes);
  }
  numCashierThreads--;
  thread_broadcast(0,0);
  thread_unlock(0);
}

void maker_thread(void* arg){
  thread_lock(0);
  int lastSandwichMade = -1;
  BulletinBoard board = *((BulletinBoard*)arg);
  while(numCashierThreads > 0){
    while(!board.isSaturated()){
			//cout << "Sleeping ";
			//debug(12345679, board.myNotes);
			thread_wait(0,0);
      //cout << "Awoken ";
			//debug(12345678, board.myNotes);
    }
    if(numCashierThreads == 0){
      break;
    }
		Note makeNote = board.pull(lastSandwichMade);
    lastSandwichMade = makeNote.sandwichNum;
		cout << "READY: cashier " << makeNote.cashierNum <<
			" sandwich " << makeNote.sandwichNum << endl;
		board.removeNote(makeNote.cashierNum);
    thread_broadcast(0,0);
  }
  thread_unlock(0);
}

void main_thread(void* arg){
	//start_preemptions(true,true,28947);
	InputData* data = (InputData*)arg;
	vector<vector<int>*>* cashier_lists = data->cashierLists;
	int board_size = data->boardSize;
	BulletinBoard* board = new BulletinBoard(board_size);
	for(int i=0; i<cashier_lists->size(); i++){
    vector<int>* cur_list = cashier_lists->operator[](i);
		Cashier* c = new Cashier(i, cur_list, board);
		thread_create(cashier_thread, c);
	}
	thread_create(maker_thread, board);
}

int main(int argc, char *argv[]){
	vector<vector<int>* >* cashier_lists = new vector<vector<int>* >();
  for(int index = 2; index < argc; index++){

  	vector<int>* cur_list = new vector<int>();
		cashier_lists->push_back(cur_list);
		ifstream the_file (argv[index]);

  	if (the_file.is_open()){
			string line;
			int i;

  		while(getline(the_file,line)){
				i = atoi(line.c_str());
				cur_list->push_back(i);
			}

  	}
	}

  int board_size = atoi(argv[1]);
	InputData* data = new InputData(board_size, cashier_lists);
  if(thread_libinit((thread_startfunc_t) main_thread,data)){
    std::cout << "thread_libinit failed\n";
    //exit(1);
  }
  return 0;
}
