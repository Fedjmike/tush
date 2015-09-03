#include "display.h"

#include <dirent.h>
#include <nicestat.h>

#include "terminal.h"
#include "paths.h"

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

static int printFilename (const char* name) {
    return   pathIsDir(name)
           ? printf_style("{%s}/", styleBlue, name)
           : printf("%s", name);
}

static int displayValueImpl (const value* result, type* dt, printf_t printf) {
    bool dry = printf == dryprintf;

    type* elementType = 0;
    vector(type*) tuple;

    /*Iterable types*/
    if (   typeIsListOf(dt, &elementType)
        || typeIsTupleOf(dt, &tuple)) {
        bool list = elementType != 0;
        char* brackets = list ? "[]" : "()";
        int length = 2;

        putchar(brackets[0]);

        for_iterable_value_indexed (i, const value* element, result, {
            if (i != 0)
                length += printf(", ");

            if (!list)
                elementType = vectorGet(tuple, i);

            if (!precond(elementType)) {
                length += valuePrint(element);
                continue;
            }

            length += displayValueImpl(element, elementType, printf);
        })

        putchar(brackets[1]);

        return length;

    } else if (typeIsKind(type_Bool, dt)) {
        return printf(valueGetInt(result) ? "true" : "false");

    } else
        return (dry ? valueGetWidthOfStr : valuePrint)(result);
}

static int displayGetWidthOfStr (const value* result, type* dt) {
    return displayValueImpl(result, dt, dryprintf);
}

static int displayValue (const value* result, type* dt) {
    return displayValueImpl(result, dt, printf);
}

static void displayGrid (vector(const char*) entries, int (*printEntry)(const char*), size_t columnWidth) {
    enum {gap = 2};
    columnWidth += gap;

    /*Work out the dimensions of the grid*/

    int windowWidth = getWindowWidth();

    int columns = windowWidth / columnWidth;
    int rows = intdiv_roundup(entries.length, columns);

    /*Print row-by-row*/

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < columns; col++) {
            const char* entry = vectorGet(entries, row + col*rows);

            if (!entry)
                break;

            size_t entrywidth = printEntry(entry);
            size_t padding = columnWidth-entrywidth;
            putnchar(' ', padding);
        }

        printf("\n");
    }
}

static void displayDirectory (const char* dirname) {
    DIR* dir = opendir(dirname);

    /*Get a listing of all the files and find the largest name*/

    vector(const char*) filenames = vectorInit(20, malloc);

    size_t largest = 0;

    for (struct dirent* entry; (entry = readdir(dir));) {
		vectorPush(&filenames, entry->d_name);
        size_t namelen = strwidth(entry->d_name);

        if (largest < namelen)
            largest = namelen;
    }

    /*Display in a grid, in alphabetical order*/
    qsort(filenames.buffer, filenames.length, sizeof(void*), qsort_cstr);
    displayGrid(filenames, printFilename, largest);

    /*Kept the dir open til now as the filenames belonged to it*/
    closedir(dir);
    vectorFree(&filenames);
}

static void displayFile (const char* filename) {
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

    if (!error && file.mode == file_dir)
        displayDirectory(filename);
}

static void displayRegular (value* result, type* dt) {
    displayValue(result, dt);
    //todo if multiline result, type on new line
    printf(" :: %s\n", typeGetStr(dt));
}

/*Display a list of files as a grid of names, going down the rows
  first and then wrapping up to the next column.*/
static void displayFileList (value* result, type* resultType) {
    /*Turn the file list into a vector of names*/
    vector(const char*) names = vectorMapInit((vectorMapper) valueGetDisplayFilename,
                                              valueGetVector(result), malloc);

    /*Find the longest filename*/

    size_t columnWidth = 0;

    for_vector (const char* name, names, {
        size_t namelen = strwidth(name);

        if (columnWidth < namelen)
            columnWidth = namelen;
    })

    /* */

    displayGrid(names, printFilename, columnWidth);

    vectorFree(&names);

    printf(" :: %s\n", typeGetStr(resultType));
}

/*Display a tuple list as a table
  (because they are tuples, the result is square)*/
static void displayTable (value* result, type* resultType, vector(type*) tuple) {
    int columns = tuple.length;

    /*Note: VLA*/
    size_t columnWidths[columns];
    memset(columnWidths, 0, sizeof(columnWidths));

    /*For each column, find the max width of any value*/

    for_iterable_value (const value* inner, result, {
        for (int col = 0; col < columns; col++) {
            const value* item = valueGetTupleNth(inner, col);
            size_t width = valueGetWidthOfStr(item);

            if (columnWidths[col] < width)
                columnWidths[col] = width;
        }
    })

    enum {gap = 2};

    /*Print it*/

    for_iterable_value (const value* inner, result, {
        for (int col = 0; col < columns; col++) {
            const value* item = valueGetTupleNth(inner, col);
            putnchar(' ', gap);

            /*Right align (i.e. print padding before the item)
              if the column is an int*/

            type* itemType = vectorGet(tuple, col);

            bool rightAlign = typeIsKind(type_Int, itemType);
            bool filename = typeIsKind(type_File, itemType);

            size_t width =   filename ? printf("%s", valueGetDisplayFilename(item))
                           : (rightAlign ? displayGetWidthOfStr : displayValue)(item, itemType);
            size_t padding = columnWidths[col] - width;

            if (rightAlign) {
                putnchar(' ', padding);
                displayValue(item, itemType);

            } else
                /*Value already printed*/
                putnchar(' ', padding);
        }

        putchar('\n');
    })

    printf(" :: %s\n", typeGetStr(resultType));
}

/*Options for lists of lists*/
enum {
    displayListList_bracesOnOwnLine = false,
    displayListList_bracesOnOwnLineIfRecursing = true
};

void displayListList (value* result, type* resultType, type* elementType, type* innerElementType, int depth) {
    vector(value*) elements = valueGetVector(result);

    /*Is the element type *also* a list of lists?*/
    type* innerInnerElementType;
    bool recursing = typeIsListOf(innerElementType, &innerInnerElementType);

    /*Put the braces on their own line, if the options say so*/
    bool bracesOnOwnLine =    displayListList_bracesOnOwnLine
                           || (   recursing
                               && displayListList_bracesOnOwnLineIfRecursing);

    putchar('[');

    if (bracesOnOwnLine) {
        putchar('\n');
        putnchar(' ', depth+1);
    }

    for_vector_indexed (i, value* element, elements, {
        if (i != 0)
            putnchar(' ', depth+1);

        if (recursing)
            displayListList(element, elementType, innerElementType, innerInnerElementType, depth+1);

        else
            displayValue(element, elementType);

        if (i < elements.length-1) {
            putchar(',');
            putchar('\n');
        }
    })

    if (bracesOnOwnLine) {
        putchar('\n');
        putnchar(' ', depth);
    }

    putchar(']');

    if (depth == 0) {
        /*If the braces are on a same line as the rest of the list
          then there is room for the type*/
        if (!bracesOnOwnLine)
            putchar('\n');

        printf(" :: %s\n", typeGetStr(resultType));
    }
}

void displayStr (value* result, type* resultType) {
    size_t length;
    const char* str = valueGetStrWithLength(result, &length);

    /*Special handling for multiline strings
       - Check for final EOL and warn if missing
       - Display without quotes, with the type on a new line*/
    if (strchr(str, '\n')) {
        bool missingEOL = str[length-1] != '\n';

        printf(missingEOL ? "%s\n :: %s\n" : "%s :: %s\n", str, typeGetStr(resultType));

        if (missingEOL)
            printf("(This string was missing a final end of line character.)\n");

    } else
        displayRegular(result, resultType);
}

void displayResult (value* result, type* resultType) {
    /*Print the value and type*/

    type *elements, *innerElements;
    vector(type*) tuple;

    if (valueIsInvalid(result))
        displayRegular(result, resultType);

    else if (typeIsListOf(resultType, &elements)) {
        /* [['a]] -- List of lists (and possibly recursive) */
        if (typeIsListOf(elements, &innerElements))
            displayListList(result, resultType, elements, innerElements, 0);

        /*Display empty or singular iterables the normal way instead one of the following*/
        else if (valueGuessIterableLength(result) <= 1)
            displayRegular(result, resultType);

        /* [File] -- File lists are displayed in an autocomplete-like grid*/
        else if (typeIsKind(type_File, elements))
            displayFileList(result, resultType);

        //todo check 'a 'b ... are simple (value and type)
        /* [('a, 'b, ...)] -- A table */
        else if (typeIsTupleOf(elements, &tuple))
            displayTable(result, resultType, tuple);

        else
            displayRegular(result, resultType);

    } else if (typeIsKind(type_Str, resultType)) {
        displayStr(result, resultType);

    } else if (typeIsKind(type_Unit, resultType)) {
        /*Don't display unit results
          These occur from statements like `let`*/

    } else {
        displayRegular(result, resultType);

        if (typeIsKind(type_File, resultType))
            displayFile(valueGetFilename(result));
    }
}
