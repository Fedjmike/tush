#include "display.h"

#include <sys/ioctl.h>
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

static unsigned int getWindowWidth (void) {
    struct winsize size;
    ioctl(0, TIOCGWINSZ, &size);

    return size.ws_col;
}

static void displayRegular (value* result, type* resultType) {
    valuePrint(result);
    printf(" :: %s\n", typeGetStr(resultType));
}

/*Display a list of files as a grid of names, going down the rows
  first and then wrapping up to the next column.*/
static void displayFileList (value* result, type* resultType) {
    /*Turn the file list into a vector of names*/
    vector(const char*) names = vectorMapInit((vectorMapper) valueGetFilename,
                                              valueGetVector(result), malloc);

    /*Find the longest filename*/

    size_t columnWidth = 0;

    for_vector (const char* name, names, {
        //todo strlen -> str actual column width
        size_t namelen = strlen(name);

        if (columnWidth < namelen)
            columnWidth = namelen;
    })

    /*Work out the dimensions of the grid*/

    enum {gap = 2};
    columnWidth += gap;

    int windowWidth = getWindowWidth();

    int columns = windowWidth / columnWidth;
    int rows = intdiv_roundup(names.length, columns);

    /*Print row-by-row*/

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            const char* name = vectorGet(names, row + col*rows);

            if (!name)
                break;

            size_t namelen = printf("%s", name);
            size_t padding = columnWidth-namelen;
            putnchar(' ', padding);
        }

        printf("\n");
    }

    vectorFree(&names);

    printf(" :: %s\n", typeGetStr(resultType));
}

/*Display a tuple list as a table
  (because they are tuples, the result is square*/
static void displayTable (value* result, type* resultType) {
    /*result :: ['tuple]*/
    type* tuple = typeGetListElements(resultType);

    int columns = typeGetTupleTypes(tuple).length;

    /*Note: VLA*/
    size_t columnWidths[columns];
    memset(columnWidths, 0, sizeof(columnWidths));

    /*Find the max size of any value for each column*/
    for_vector (value* inner, valueGetVector(result), {
        for_vector_indexed (col, value* item, valueGetVector(inner), {
            size_t width = valueGetWidthOfStr(item);

            if (columnWidths[col] < width)
                columnWidths[col] = width;
        })
    })

    enum {gap = 2};

    /*Print it*/
    
    for_vector (value* inner, valueGetVector(result), {
        for_vector_indexed (col, value* item, valueGetVector(inner), {
            size_t width = valuePrint(item);

            size_t padding = columnWidths[col] + gap - width;
            putnchar(' ', padding);
        })

        putchar('\n');
    })

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

    if (valueIsInvalid(result))
        displayRegular(result, resultType);

    else if (typeIsList(resultType)) {
        type* elements = typeGetListElements(resultType);

        /*Display empty or singular iterables the normal way instead one of the following*/
        if (valueGetVector(result).length <= 1)
            displayRegular(result, resultType);

        /* [File] -- File lists are displayed in an autocomplete-like grid*/
        else if (typeIsKind(type_File, elements))
            displayFileList(result, resultType);

        /* [('a, 'b, ...)] -- A table */
        else if (typeIsKind(type_Tuple, elements))
            displayTable(result, resultType);

        else
            displayRegular(result, resultType);

    } else {
        displayRegular(result, resultType);

        if (typeIsKind(type_File, resultType))
            displayFileStats(valueGetFilename(result));
    }
}
