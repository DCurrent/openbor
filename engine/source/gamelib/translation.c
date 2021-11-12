#include "openbor.h"
#include "List.h"
#include "translation.h"

static List *transList = NULL;


static char *__consume_str(char *buf, ptrdiff_t *pos, size_t size)
{
    int stron = 0;
    int len = 0;
    char *result = malloc(1000); //should be enough
    result[0] = 0;
    while(*pos < size)
    {
        if(!stron)
        {
            if(buf[*pos] == '\"')
            {
                stron = 1;
            }
            else if(buf[*pos] != ' ' && buf[*pos] != '\t' && buf[*pos] != '\r' && buf[*pos] != '\n')
            {
                --*pos;
                break;
            }
        }
        else
        {
            if(buf[*pos] == '\"')
            {
                stron = 0;
            }
            else if(buf[*pos] == '\\' && *pos < size - 2)
            {
                switch (buf[1 + *pos])
                {
                case 's':
                    result[len++] = ' ';
                    break;
                case 'r':
                    result[len++] = '\r';
                    break;
                case 'n':
                    result[len++] = '\n';
                    break;
                case 't':
                    result[len++] = '\t';
                    break;
                case '\"':
                    result[len++] = '\"';
                    break;
                case '\'':
                    result[len++] = '\'';
                    break;
                default:
                    result[len++] = '\\';
                    break;
                }
                ++*pos;
            }
            else
            {
                result[len++] = buf[*pos];
            }
        }

        ++*pos;
    }

    if(stron)
    {
        borShutdown(1, "Unterminated string in translation.txt.");
    }
    //TODO: free pointers
    result[len] = 0;
    return result;
}

static int ob_loadtrans()
{
    ptrdiff_t pos;
    size_t size;
    char *buf = NULL;
    char *id = NULL;
    char *str = NULL;
    char *tmp = NULL;
    int isid = 0;
    // Read file

    /*
        Kratus (10-2021) Added an alternative location for the translation file, now it's possible to use in an external folder
        Now the modder can load exported translation files by using "filestream" script functions
        Useful for creating custom translations without unpack the game
        The default engine translation location will be maintained for backward compatibility
        
        Kratus (11-2021) Inverted the path priority, now the external file will override the internal file
        Useful to maintain the english translation intact inside the pak file if no other language file is found in the external path
        Otherwise you will need to rollback the english file every time another language is used and then removed
        This operation is needed only if the english translation file uses some custom menu texts for english language too
    */
    if(buffer_pakfile("translation/translation.txt", &buf, &size) != 1)
    {
        goto default_file;
    }
    else
    {
        goto proceed;
    }

default_file:

    if(buffer_pakfile("data/translation.txt", &buf, &size) != 1)
    {
        return 0;
    }
    else
    {
        goto proceed;
    }

proceed:
    //printf("Loading translation table\n");

    pos = 0;
    while(pos < size)
    {
        if(buf[pos] == 'm' && starts_with(buf + pos, "msgid"))
        {
            isid = 1;
        }
        else if (buf[pos] == 'm' && starts_with(buf + pos, "msgstr"))
        {
            isid = 0;
        }
        else if(buf[pos] == '\"')
        {
            tmp = __consume_str(buf, &pos, size);
            if(isid)
            {
                id = tmp;
            }
            else
            {
                str = tmp;
                ob_addtrans(id, str);
                //printf("\n%s\n%s\n", id, str);
                if(id)
                {
                    free(id);
                }
                id = NULL;
                if(str)
                {
                    free(str);
                }
                str = NULL;
            }
        }

        pos++;
    }

    if(buf)
    {
        free(buf);
    }
    buf = NULL;
    if(id)
    {
        free(id);
    }
    id = NULL;
    if(str)
    {
        free(str);
    }
    str = NULL;

    return 1;
}

void ob_inittrans()
{
    transList = malloc(sizeof(List));
    List_Init(transList);
    if(!ob_loadtrans())
    {
        ob_termtrans();
    }
}

void ob_termtrans()
{
    int i, size;
    if(NULL == transList)
    {
        return;
    }
    PFOREACH(
        transList,
        free(List_Retrieve(transList));
    );
    List_Clear(transList);
    free(transList);
    transList = NULL;
}

void ob_addtrans(char *id, char *str)
{
    if(!id || !str)
    {
        return ;
    }
    if(!str[0])
    {
        return;    //skip empty translation
    }
    List_GotoLast(transList);
    List_InsertAfter(transList, (void *)NAME(str), id);
}

char *ob_gettrans(char *id)
{
    static char *lastid = NULL;
    static char *laststr = NULL;

    if(lastid == id)
    {
        return laststr;    //deal with some menu text macros
    }

    lastid = id;

    if(transList && List_FindByName(transList, id))
    {
        return (laststr = (char *)List_Retrieve(transList));
    }


    return (laststr = id);
}


