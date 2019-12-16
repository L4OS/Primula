#include "pascal_generator.h"

void PascalGenerator::GenerateStaticData(static_data_t * data, bool last, bool selfformat)
{
    switch (data->type)
    {
    case lt_openblock:
        //structure_t * structure = (structure_t *)data->nested;
        if (selfformat)
            Write("(");
        else
            PrintOpenBlock();
        for (
            std::list<struct static_data*>::iterator compound = data->nested->begin();
            compound != data->nested->end();
            ++compound)
        {
            if (!selfformat)
                IndentWrite("");
            if (*compound == nullptr)
            {
                Write("/* something goes wrong */0, ");
            }
            else
            {
                bool last = (*compound == data->nested->back());
                GenerateStaticData(*compound, last, true);
            }
        }
        if (selfformat)
            Write(")");
        else
            PrintCloseBlock();
        break;
    case lt_string:
        Write("'%s'", data->p_char);
        break;
    case lt_integer:
        Write("%d", data->s_int);
        break;
    case lt_openindex:
        if (data->nested->size() == 0)
            Write("\n /* Nested data size is not defined */\n");
        Write("( ");
        {
            std::list<struct static_data*>::iterator term;
            for (
                term = data->nested->begin();
                term != data->nested->end();
                ++term)
            {
                bool last = (*term == data->nested->back());
                GenerateStaticData(*term, last, false);
            }
        }
        Write(" )");
        break;
    default:
        throw "Static data type not parsed";
    }
    if (!last)
        Write(", ");
}

