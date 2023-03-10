#include "matrix/formats/csr.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

double getCSR(Matrix *self, int r, int c){

    CSRData *data = (CSRData *)self->data;
    int startColIndex, endColIndex;

    startColIndex = data ->firstColOfRowIndexes[r];
    endColIndex = data ->firstColOfRowIndexes[r + 1];

    // we search for the desired column in the range [startColIndex, endColIndex]
    for (int i = startColIndex; i < endColIndex; i ++){
        if (data ->columns[i] == c){
            return data ->values[i];
        }
    }

    return 0;
}

NotZeroElement *getNonZeroCSR(Matrix *self, int pos){

    CSRData *data = (CSRData *)self->data;
    int startColIndex, endColIndex;
    int currentPos = 0;
    NotZeroElement *element = NULL;
    for (int r = 0; r < self ->rows; r++){
        startColIndex = data ->firstColOfRowIndexes[r];
        endColIndex = data ->firstColOfRowIndexes[r + 1];

        for(int c = startColIndex; c < endColIndex; c ++, currentPos ++){
            if (currentPos == pos){
                element = (NotZeroElement *)calloc(1, sizeof(NotZeroElement));
                element ->row = r;
                element ->col = data ->columns[c];
                element ->value = data ->values[c];
                goto end;
            }
        }
    }
    end:
    return element;
}


/**
 * Swap macro. Using a macro gains a lot of performance. (99% --> 59.53% of execution time)
*/
#define swap(a, b, t)\
    do{\
        t = a;\
        a = b;\
        b = t;\
    }while(0)

#define shiftRight(array, n, startPos, eType, ret)\
    do {\
        ret = false;\
        eType temp;\
        for (int i = startPos + 1; i < n; i ++){\
            ret = true;\
            swap(((eType *)array)[startPos], ((eType *)array)[i], temp);\
        }\
    } while (0)

int putCSR(Matrix *self, int r, int c, double value){

    CSRData *data = (CSRData *)self->data;
    int startColIndex, endColIndex, colsOfRow;
    bool hasShifted;

    /**
    * TODO: Weakness disgusts me, handle zero values! >:(
    * When trying to put zero value, it should overwrite the previous value,
    * effectively canceling it from the matrix. 
    */
    // we refuse zero values since it's too complicated to handle them (its 2:23 AM I just want to go to sleep lol)
    ON_ERROR_LOG_AND_RETURN(value == 0, -1, "Cannot put zero values in a CSR matrix format (position: %d, %d)\n", r, c);
    
    if (r >= self ->rows){
        
        // we make room for the new row
        // new size = new num of rows + 1 since the last element is always the num of non-zero elements
        // new num of rows = desired row index + 1
        data ->numCompressedRows = r + 1 + 1;
        data ->firstColOfRowIndexes = (int *)reallocarray(data ->firstColOfRowIndexes, data ->numCompressedRows, sizeof(int));
        
        // we fill the new positions by repeating the old last value
        // (assuming that the last value is always the num of non-zero elements)
        for (int i = self ->rows; i < data ->numCompressedRows; i ++){
            data ->firstColOfRowIndexes[i] = self ->numNonZero;
        }
        self ->rows = r + 1;
    }

    // get range of columns for the given row.
    // we always avoid buffer overflow since the last element of rows array always
    // contains the number of non-zero elements
    startColIndex = data ->firstColOfRowIndexes[r];
    endColIndex = data ->firstColOfRowIndexes[r + 1];
    colsOfRow = endColIndex - startColIndex;

    // update the element with the given column, if exists
    for (int i = startColIndex; i < endColIndex; i ++){
        if (data ->columns[i] == c){
            data ->values[i] = value;
            self ->numNonZero ++;
            data ->firstColOfRowIndexes[data ->numCompressedRows - 1] = self ->numNonZero;
            return 0;
        }
    }

    // No value has been registered at given coordinates in the past.
    // We must create a new space for the new value.
    if (self ->numNonZero + 1 > data ->valuesCapacity){
        data ->values = (double *)reallocarray(data ->values, data ->valuesCapacity + 1, sizeof(double));
        data ->columns = (int *)reallocarray(data ->columns, data ->valuesCapacity + 1, sizeof(int));
        data ->valuesCapacity ++;
        self->cols = data ->valuesCapacity;
    }
    
    // we could need to shift the columns and values of the next row to the right (not always)
    shiftRight(data ->columns, data ->valuesCapacity, endColIndex, int, hasShifted);
    if (hasShifted){
        shiftRight(data ->values, data ->valuesCapacity, endColIndex, double, hasShifted);
        // if we actually shifted, we must adjust the firstColOfRowIndexes array
        for (int i = r + 1; i < data ->numCompressedRows - 1; i ++){
            data ->firstColOfRowIndexes[i] ++;
        }
    }
    // put the new value in the newly created space
    data ->columns[endColIndex] = c;
    data ->values[endColIndex] = value;
    self ->numNonZero ++;
    data ->firstColOfRowIndexes[data ->numCompressedRows - 1] = self ->numNonZero;

    
    return 0;
}

void freeCSR(Matrix *self){

    CSRData *data = (CSRData *)self->data;
    free(data ->columns);
    free(data ->values);
    free(data ->firstColOfRowIndexes);
    free(data);
    free(self);
}

void printCSR(Matrix *self){

    CSRData *data = (CSRData *)self->data;
    int startColIndex, endColIndex, colsOfRow;

    for (int r = 0; r < data ->numCompressedRows - 1; r ++){
        printf("row %d: ", r);
        startColIndex = data ->firstColOfRowIndexes[r];
        endColIndex = data ->firstColOfRowIndexes[r + 1];
        for (int c = startColIndex; c < endColIndex; c ++){
            printf("(%d, %f) ", data ->columns[c], data ->values[c]);
        }
        printf("\n");
    }
}

long getSizeCSR(Matrix *self){
    CSRData *data = (CSRData *)self->data;
    return sizeof(int) * data ->numCompressedRows + sizeof(int) * self ->numNonZero + sizeof(double) * self ->numNonZero;
}

Matrix *cloneEmptyCSR(Matrix *self){

    return newMatrixCSR();
}

Matrix *newMatrixCSR(){

    Matrix *self = newMatrix();
    CSRData *data = (CSRData *)calloc(1, sizeof(CSRData));
    
    self -> formatName = "CSR";
    self -> data = data;

    self -> get = getCSR;
    self -> put = putCSR;
    self -> print = printCSR;
    self -> getSize = getSizeCSR;
    self -> cloneEmpty = cloneEmptyCSR;
    self ->getNonZero = getNonZeroCSR;

    return self;
}