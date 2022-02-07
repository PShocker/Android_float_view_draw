#ifndef AOBSCAN_H
#define AOBSCAN_H


//两内存地址逐一比较
bool Memcmp(unsigned const char *target,unsigned const char *pattern, int Len)
{
    for (int i = 0; i < Len; i++)
    {
        if (target[i] == pattern[i])
        {
            continue;
        }
        else
        {
            return false;
        }
    }
    return true;
}

//特征码搜索
int AOBScan(unsigned const char *target, int tLen, unsigned const char *pattern, int pLen)
{
    if (tLen<pLen)
    {
        return -1;
    }
    for (int i = 0; i < tLen; i++)
    {
        if (Memcmp(target + i, pattern, pLen))
        {
            return i;
        }
    }
    return -1;
}

#endif //AOBSCAN_H