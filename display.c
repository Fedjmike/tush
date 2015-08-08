#include "display.h"

#include <nicestat.h>

#include "type.h"
#include "value.h"

enum {
    useSIUnitNames = false,
    sizeMagnitudes = 1024
};

const char* units[] = {"bytes", "kB", "MB", "GB", "TB"};
const char* unitsSI[] = {"bytes", "kiB", "MiB", "GiB", "TiB"};

static void printSizeNicely (size_t size) {
    size_t magnitude = 1;
    int orderOfMag = 0;

    /*Find the greatest order of magnitude less than the size*/
    while (size > sizeMagnitudes*magnitude) {
        magnitude *= sizeMagnitudes;
        orderOfMag++;

        if (orderOfMag >= 4)
            break;
    }

    double relativeSize = (double) size / (double) magnitude;
    const char* unit = (useSIUnitNames ? unitsSI : units)[orderOfMag];

    /*Select at most 2 decimal digits
      (That is, 3sf most of the time)*/
    int digitsAfterPoint =   relativeSize > 100 ? 0
                           : relativeSize > 10 ? 1 : 2;

    printf("%.*f %s", digitsAfterPoint, relativeSize, unit);
}

static void displayFileStats (const char* filename) {
    stat_t file;
    staterr error = nicestat(filename, &file);

    printf("(");

    if (!error) {
        if (file.mode == file_regular)
            printSizeNicely(file.size);

        else
            printf("A %s", fmode_getstr(file.mode));

    } else {
        switch (error) {
        case staterr_notexist:
            printf("This file does not exist");
            break;

        case staterr_notdir:
            printf("This file has an invalid path");
            break;

        case staterr_access:
            printf("You do not have permission to access this path");
            break;

        default:
            ; /*Better to say nothing than say something wrong*/
        }
    }

    printf(")\n");
}

static int intdiv_roundup (int dividend, int divisor) {
    return (dividend - 1) / divisor + 1;
}

/*Display a list of files as a grid of names, going down the rows
  first and then wrapping up to the next column.*/
static void displayFileList (value* result, type* resultType) {
    /*Turn the file list into a vector of names*/
    vector(const char*) names = vectorMapInit((vectorMapper) valueGetFilename,
                                              valueGetVector(result), malloc);


    /*Dimensions of the grid*/
    int length = names.length;
    enum {columns = 4, gap = 3};
    int rows = intdiv_roundup(length, columns);

    /*Work out the widths of the column
      (max width of any filename plus the gap)*/

    size_t columnWidth[columns] = {};

    for (int i = 0, col = 0; col < columns; col++) {
        for (int row = 0; i < length && row < rows; i++, row++) {
            const char* name = vectorGet(names, i);
            //todo cache strlen
            //strlen -> str actual column wdith
            size_t namelen = strlen(name);

            if (columnWidth[col] < namelen+gap)
                columnWidth[col] = namelen+gap;
        }
    }

    /*Print row-by-row*/

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            const char* name = vectorGet(names, row + col*rows);

            if (!name)
                break;

            printf("%s", name);

            size_t namelen = strlen(name);
            size_t padding = columnWidth[col]-namelen;

            for (unsigned int i = 0; i < padding; i++)
                putchar(' ');
        }

        printf("\n");
    }

    vectorFree(&names);

    printf(" :: %s\n", typeGetStr(resultType));
}

void displayResult (value* result, type* resultType) {
    /*If the result is () -> 'a ...*/
    if (typeUnitAppliesToFn(resultType)) {
        printf("(A value of %s has been automatically applied.)\n", typeGetStr(resultType));

        /*Apply unit to the result and display that instead*/
        resultType = typeGetFnResult(resultType);
        result = valueCall(result, valueCreateUnit());
    }

    /*Print the value and type*/

    if (typeIsList(resultType) && typeIsKind(typeGetListElements(resultType), type_File))
        displayFileList(result, resultType);

    else {
        valuePrint(result);
        printf(" :: %s\n", typeGetStr(resultType));

        if (typeIsKind(resultType, type_File))
            displayFileStats(valueGetFilename(result));
    }
}
