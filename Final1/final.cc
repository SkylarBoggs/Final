/**
 * @file final.cc
 * @author Skylar Boggs (sb667617@ohio.edu)
 * @brief program to find line count and number of characters
 * @version 0.1
 * @date 2020-12-03
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>
#include <cctype>
#include <fstream>

using namespace std;

/**
 * @brief returns the number of lines in a string 
 * 
 * @return line_count 
 */
int countLine(string x){
    int count = 0;
    char chars;
    for(int i = 0; i < x.length(); i++){
        cout << "why2" ;
        chars = x.at(i);
        if (chars == '\n'){
            count++;
        }
        count++;
    }
    return count;
    cout << "Line count is: " << count <<endl;
}

/**
 * @brief returns the number of characters in a string 
 * 
 * @return char_count 
 */

int countChar(string x){
    int count1 = 0;
    char chars;
    for(int i = 0; i < x.length(); i++){
        cout << "why3" ;
        chars = x.at(i);
        if (chars == ' '){
            count1--;
        }
        count1++;
    }
    return count1;
    cout << "character count is: " << count1 << endl;
}


int main(int argc, char* argv[]){
    //no command line preferences = countline(ohio university") and countchar(athens)
    //one command line specified = file name

    ifstream ifs;
    ofstream ofs;
    int count, count1;
    string input, input1, input2;
    string lines, chars;
    input1 = argv[1];
    input2 = argv[2];

    if (argc < 2 && argc > 0){
        ifs.open(input1); 
        while(!ifs.eof()){
            cout << "why";
            getline(ifs, input);
            countLine(input);
            countChar(input);
		}
    }
    if (argc == 0){
        countLine("Ohio University");
        countChar("Athens");
        cout << "why1" ;
    }
    ifs.close();

}





    


