/***************************************************************************

 file        : ac3dgroup.cpp
 created     : Fri Apr 18 23:11:36 CEST 2003
 copyright   : (C) 2003 by Christophe Guionneau
 version     : $Id: ac3dgroup.cpp 5807 2014-08-21 02:56:49Z beaglejoe $

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/** @file   

 @author Christophe Guionneau
 @version    $Id: ac3dgroup.cpp 5807 2014-08-21 02:56:49Z beaglejoe $
 */

#include <cstdio>
#include <cstdlib>
#include <ctype.h>
#include <cstring>
#include <cmath>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <portability.h>
#include "accc.h"

extern int printOb(ob_t * ob);
extern mat_t * root_material;
extern void smoothTriNorm(ob_t * object);
void reorder(ob_t * ob, ob_t * ob2, double *textarray, tcoord_t *vertexarray);
void collapseTextures(ob_t * ob0, ob_t * ob1, ob_t * ob2, ob_t * ob3);
void loadAndGroup(char *OutputFileName)
{
    ob_t * ob0 = NULL;
    ob_t * ob1 = NULL;
    ob_t * ob2 = NULL;
    ob_t * ob3 = NULL;
    ob_t * tmpob = NULL;
    ob_t * tmpob2 = NULL;
    mat_t * tmat = NULL;
    extern FILE * ofile;
    int num_tkmn = 0;
    ob_groups_t * array_groups;
    int good_group = 0;
    int i = 0;
    double dist = 0;

    /* disable object splitting during load. We split them
     * after merging the texture channels.
     */
    splitObjectsDuringLoad = 0;

    if (fileL0)
    {
        fprintf(stderr, "\nloading file %s\n", fileL0);
        loadAC(fileL0);
        ob0 = root_ob;
        root_ob = NULL;
    }
    if (fileL1)
    {
        fprintf(stderr, "\nloading file %s\n", fileL1);
        loadAC(fileL1);
        ob1 = root_ob;
        root_ob = NULL;
    }
    if (fileL2)
    {
        fprintf(stderr, "\nloading file %s\n", fileL2);
        loadAC(fileL2);
        ob2 = root_ob;
        root_ob = NULL;
    }
    if (fileL3)
    {
        fprintf(stderr, "\nloading file %s\n", fileL3);
        loadAC(fileL3);
        ob3 = root_ob;
        root_ob = NULL;
    }
    /* now collapse the texture and texture  arrays of 1 2 3 in 0 */

    smoothTriNorm(ob0);

    printf("collapsing textures \n");
    fprintf(stderr, "\ncollapsing textures\n");

    collapseTextures(ob0, ob1, ob2, ob3);

    ob0 = splitObjects(ob0);

    /* now make groups from ob0 */

    fprintf(stderr, "making groups\n");
    tmpob = ob0;
    num_tkmn = 0;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strnicmp(tmpob->name, "tkmn", 4) && strcmp(tmpob->type, "group"))
        {
            tmpob = tmpob->next;
            num_tkmn++;
            continue;
        }

        tmpob = tmpob->next;
    }

    printf("found %d tkmn\n", num_tkmn);
    if (num_tkmn == 0)
    {
        fprintf(stderr,
                "\nERROR: cannot find any object tkmn for grouping\nAborting\n");
        exit(-1);
    }

    i = 0;
    tmpob = ob0;
    array_groups = (ob_groups_t *) malloc(sizeof(ob_groups_t) * num_tkmn);
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strnicmp(tmpob->name, "tkmn", 4) && strcmp(tmpob->type, "group"))
        {
            array_groups[i].tkmn = tmpob;
            array_groups[i].numkids = 1;
            array_groups[i].name = tmpob->name;
            array_groups[i].tkmnlabel = atoi(tmpob->name + 4);
            array_groups[i].kids = NULL;
            array_groups[i].kids0 = NULL;
            array_groups[i].kids1 = NULL;
            array_groups[i].kids2 = NULL;
            array_groups[i].kids3 = NULL;
            array_groups[i].numkids0 = 0;
            array_groups[i].numkids1 = 0;
            array_groups[i].numkids2 = 0;
            array_groups[i].numkids3 = 0;
            tmpob = tmpob->next;
            i++;
            continue;
        }
        tmpob = tmpob->next;
    }

    fprintf(stderr, "dispatching objects in groups\n");
    tmpob = ob0;
    while (tmpob != NULL)
    {
        if (tmpob->name == NULL)
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "root"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strcmp(tmpob->name, "world"))
        {
            tmpob = tmpob->next;
            continue;
        }
        if (!strnicmp(tmpob->name, "tkmn", 4))
        {
            tmpob = tmpob->next;
            continue;
        }

        good_group = -1;
        tmpob->dist_min = 1000000;
        for (i = 0; i < num_tkmn; i++)
        {
            dist = findDistmin(array_groups[i].tkmn, tmpob);
            if (dist < tmpob->dist_min)
            {
                tmpob->dist_min = dist;
                good_group = i;
            }
            if (!strnicmp(tmpob->name, "t0RB", 4)
                    || !strnicmp(tmpob->name, "t1RB", 4)
                    || !strnicmp(tmpob->name, "t2RB", 4)
                    || !strnicmp(tmpob->name, "tkRS", 4)
                    || !strnicmp(tmpob->name, "t0LB", 4)
                    || !strnicmp(tmpob->name, "t1LB", 4)
                    || !strnicmp(tmpob->name, "t2LB", 4)
                    || !strnicmp(tmpob->name, "tkLS", 4)
                    || !strnicmp(tmpob->name, "BOLt", 4)
                    || !strnicmp(tmpob->name, "BORt", 4))
            {
                if (atoi(tmpob->name + 4) == array_groups[i].tkmnlabel)
                {
                    printf("object %s is forced in group %d \n", tmpob->name,
                            array_groups[i].tkmnlabel);
                    good_group = i;
                    break;
                }
            }
        }
        if (good_group == -1)
        {
            printf("an object in no group %s \n", tmpob->name);
            tmpob = tmpob->next;
            continue;
        }
        printf("object %s is going to group %s  at dist=%f\n", tmpob->name,
                array_groups[good_group].name, sqrt(tmpob->dist_min));
        if (array_groups[good_group].kids == NULL)
        {
            array_groups[good_group].kids = tmpob;
            tmpob = tmpob->next;
            array_groups[good_group].numkids++;
            array_groups[good_group].kids->next = NULL;
        }
        else
        {
            tmpob2 = array_groups[good_group].kids;
            array_groups[good_group].kids = tmpob;
            tmpob = tmpob->next;
            array_groups[good_group].kids->next = tmpob2;
            array_groups[good_group].numkids++;
        }

        /*tmpob=tmpob->next;*/
    }
    /* now each tkmn group contains the list of its kids */

    /* for all tkmn groups the kids are dispatched in the different group levels */

#define INSERTINGROUP(kids,ob)   {\
     ob->next=kids;\
     kids=ob;\
   }

    for (i = 0; i < num_tkmn; i++)
    {
        ob_t * tmpobnext;
        tmpob = array_groups[i].kids;
        printf("grouping level for %s\n", array_groups[i].name);
        while (tmpob != NULL)
        {
            tmpobnext = tmpob->next;
            if (tmpob->name == NULL)
            {
                tmpob = tmpobnext;
                continue;
            }
            if (!strnicmp(tmpob->name, "tkrb", 4))
            {
                array_groups[i].numkids--;
                array_groups[i].numkids0++;
                INSERTINGROUP(array_groups[i].kids0, tmpob);
                printf("inserting %s in group 0 of %s\n", tmpob->name,
                        array_groups[i].name);
                tmpob = tmpobnext;
                continue;
            }
            if (!strnicmp(tmpob->name, "tklb", 4))
            {
                array_groups[i].numkids--;
                array_groups[i].numkids0++;
                INSERTINGROUP(array_groups[i].kids0, tmpob);
                printf("inserting %s in group 0 of %s\n", tmpob->name,
                        array_groups[i].name);
                tmpob = tmpobnext;
                continue;
            }
            if (!strnicmp(tmpob->name, "tkrs", 4))
            {
                array_groups[i].numkids--;
                array_groups[i].numkids0++;
                INSERTINGROUP(array_groups[i].kids0, tmpob);
                printf("inserting %s in group 0 of %s\n", tmpob->name,
                        array_groups[i].name);
                tmpob = tmpobnext;
                continue;
            }
            if (!strnicmp(tmpob->name, "tkls", 4))
            {
                array_groups[i].numkids--;
                array_groups[i].numkids0++;
                INSERTINGROUP(array_groups[i].kids0, tmpob);
                printf("inserting %s in group 0 of %s\n", tmpob->name,
                        array_groups[i].name);
                tmpob = tmpobnext;
                continue;
            }

            if (tmpob->dist_min < d1 * d1)
            {
                array_groups[i].numkids--;
                array_groups[i].numkids1++;
                INSERTINGROUP(array_groups[i].kids1, tmpob);
                printf("inserting %s in group 1 of %s\n", tmpob->name,
                        array_groups[i].name);
            }
            else if (tmpob->dist_min < d2 * d1)
            {
                array_groups[i].numkids--;
                array_groups[i].numkids2++;
                INSERTINGROUP(array_groups[i].kids2, tmpob);
                printf("inserting %s in group 2 of %s\n", tmpob->name,
                        array_groups[i].name);
            }
            else if (tmpob->dist_min < d3 * d3)
            {
                array_groups[i].numkids--;
                array_groups[i].numkids3++;
                INSERTINGROUP(array_groups[i].kids3, tmpob);
                printf("inserting %s in group 3 of %s\n", tmpob->name,
                        array_groups[i].name);
            }
            else
            {
                printf("cannot insert object %s in group %s\n", tmpob->name,
                        array_groups[i].name);
            }
            /*if (!strnicmp(tmpob->name, "tk",2)){
             tmpob2=tmpob;
             tmpob=tmpob->next;
             continue;
             }*/

            tmpob = tmpobnext;
        }
        if (array_groups[i].numkids == 0)
            array_groups[i].kids = NULL;
        printf("in group %s\n", array_groups[i].name);
        printf("    found in l0  %d\n", array_groups[i].numkids0);
        printf("    found in l1  %d\n", array_groups[i].numkids1);
        printf("    found in l2  %d\n", array_groups[i].numkids2);
        printf("    found in l3  %d\n", array_groups[i].numkids3);
        printf("    staying kids  %d\n", array_groups[i].numkids - 1); /* because of the tkmn not moved */

    }

    /*#ifdef NEWSRC*/
    for (i = 0; i < num_tkmn; i++)
    {
        int red = 0;
        if (array_groups[i].numkids3 > 0)
        {
            red = mergeSplitted(&(array_groups[i].kids3));
            array_groups[i].numkids3 -= red;
        }
        if (array_groups[i].numkids2 > 0)
        {
            red = mergeSplitted(&(array_groups[i].kids2));
            array_groups[i].numkids2 -= red;
        }
        if (array_groups[i].numkids1 > 0)
        {
            red = mergeSplitted(&(array_groups[i].kids1));
            array_groups[i].numkids1 -= red;
        }
    }
    /*#endif*/

    fprintf(stderr, "writing destination file %s\n", OutputFileName);

    if ((ofile = fopen(OutputFileName, "w")) == NULL)
    {
        fprintf(stderr, "failed to open %s\n", OutputFileName);
        return;
    }
    fprintf(ofile, "AC3Db\n");
    tmat = root_material;
    while (tmat != NULL)
    {
        if (strcmp(tmat->name, "root") == 0)
        {
            tmat = tmat->next;
            continue;
        }
        fprintf(ofile,
                "MATERIAL %s rgb %1.2f %1.2f %1.2f amb %1.2f %1.2f %1.2f emis %1.2f %1.2f %1.2f spec %1.2f %1.2f %1.2f shi %3d trans 0 \n",
                tmat->name, tmat->rgb.r, tmat->rgb.g, tmat->rgb.b, tmat->amb.r,
                tmat->amb.g, tmat->amb.b, tmat->emis.r, tmat->emis.g,
                tmat->emis.b, tmat->spec.r, tmat->spec.g, tmat->spec.b,
                (int) tmat->shi);
        /*(int)tmat->trans);*/
        tmat = tmat->next;
    }

    fprintf(ofile, "OBJECT world\n");
    fprintf(ofile, "kids %d\n", num_tkmn);

    for (i = 0; i < num_tkmn; i++)
    {
        int numg = 0;
        fprintf(ofile, "OBJECT group\n");
        fprintf(ofile, "name \"%s_g\"\n", array_groups[i].tkmn->name);
        numg = (array_groups[i].kids3 == 0 ? 0 : 1)
                + (array_groups[i].kids2 == 0 ? 0 : 1)
                + (array_groups[i].kids1 == 0 ? 0 : 1) + 1;
        fprintf(ofile, "kids %d\n", numg);
        /*printOb(array_groups[i].tkmn);*/

        if (array_groups[i].numkids3 > 0)
        {
            fprintf(ofile, "OBJECT group\n");
            fprintf(ofile, "name \"___%s_gl3\"\n", array_groups[i].tkmn->name);
            fprintf(ofile, "kids %d\n", array_groups[i].numkids3);
            printf("writing group: ___%s_gl3\n", array_groups[i].tkmn->name);
            tmpob = array_groups[i].kids3;
            while (tmpob != NULL)
            {

                printOb(tmpob);
                printf("%s\n", tmpob->name);
                tmpob = tmpob->next;
            }
        }

        if (array_groups[i].numkids2 > 0)
        {
            fprintf(ofile, "OBJECT group\n");
            fprintf(ofile, "name \"%%___%s_gl2\"\n",
                    array_groups[i].tkmn->name);
            fprintf(ofile, "kids %d\n", array_groups[i].numkids2);
            printf("writing group: ___%s_gl2\n", array_groups[i].tkmn->name);
            tmpob = array_groups[i].kids2;
            while (tmpob != NULL)
            {
                printOb(tmpob);
                printf("%s\n", tmpob->name);
                tmpob = tmpob->next;
            }
        }
        if (array_groups[i].numkids1 > 0)
        {
            fprintf(ofile, "OBJECT group\n");
            fprintf(ofile, "name \"___%s_gl1\"\n", array_groups[i].tkmn->name);
            fprintf(ofile, "kids %d\n", array_groups[i].numkids1);
            printf("writing group: ___%s_gl1\n", array_groups[i].tkmn->name);
            tmpob = array_groups[i].kids1;
            while (tmpob != NULL)
            {
                printOb(tmpob);
                printf("%s\n", tmpob->name);
                tmpob = tmpob->next;
            }
        }

        /* there is always a group 0 with the tkmn at leat */
        fprintf(ofile, "OBJECT group\n");
        fprintf(ofile, "name \"___%s_gl0\"\n", array_groups[i].tkmn->name);
        fprintf(ofile, "kids %d\n", array_groups[i].numkids0 + 1);
        printf("writing group: ___%s_gl0\n", array_groups[i].tkmn->name);
        tmpob = array_groups[i].kids0;
        while (tmpob != NULL)
        {
            printOb(tmpob);
            printf("%s\n", tmpob->name);
            tmpob = tmpob->next;
        }
        printOb(array_groups[i].tkmn);
    }

    return;
}

void reorder(ob_t * ob, ob_t * ob2, double *textarray, tcoord_t *vertexarray)
{
    int i = 0;
    int j = 0;
    int k = 0;

    for (i = 0; i < ob->numvert; i++)
    {
        if ((ob->vertex[i].x != ob2->vertex[i].x)
                || (ob->vertex[i].y != ob2->vertex[i].y)
                || (ob->vertex[i].z != ob2->vertex[i].z))
        {
            for (j = 0; j < ob->numvert; j++)
            {
                if ((ob->vertex[i].x == ob2->vertex[i].x)
                        && (ob->vertex[i].y == ob2->vertex[i].y)
                        && (ob->vertex[i].z == ob2->vertex[i].z))
                {
                    double tx, ty, tz;
                    double tu, tv;
                    int tindice;
                    int tsaved;
                    double text;

                    k++;

                    tx = ob2->vertex[i].x;
                    ty = ob2->vertex[i].y;
                    tz = ob2->vertex[i].z;
                    ob2->vertex[i].x = ob2->vertex[j].x;
                    ob2->vertex[i].y = ob2->vertex[j].y;
                    ob2->vertex[i].z = ob2->vertex[j].z;
                    ob2->vertex[j].x = tx;
                    ob2->vertex[j].y = ty;
                    ob2->vertex[j].z = tz;

                    tu = vertexarray[i].u;
                    tv = vertexarray[i].v;
                    tindice = vertexarray[i].indice;
                    tsaved = vertexarray[i].saved;
                    vertexarray[i].u = vertexarray[j].u;
                    vertexarray[i].v = vertexarray[j].v;
                    vertexarray[i].indice = vertexarray[j].indice;
                    vertexarray[i].saved = vertexarray[j].saved;
                    vertexarray[j].u = tu;
                    vertexarray[j].v = tv;
                    vertexarray[j].saved = tsaved;
                    vertexarray[j].indice = tindice;

                    text = textarray[i * 2];
                    textarray[i * 2] = textarray[j * 2];
                    textarray[j * 2] = text;
                    text = textarray[i * 2 + 1];
                    textarray[i * 2 + 1] = textarray[j * 2 + 1];
                    textarray[j * 2 + 1] = text;

                }

            }
        }

    }
    printf("%s : reordered %d points\n", ob->name, k);
    return;
}

/** Returns 0 if the given object has no name or is root, world or a group.
 */
int isNamedAndPolygon(ob_t * ob)
{
    if (ob->name == NULL)
        return 0;
    if (!strcmp(ob->name, "root"))
        return 0;
    if (!strcmp(ob->name, "world"))
        return 0;
    if (ob->type != NULL
    && !strcmp(ob->type, "group"))
        return 0;

    return 1;
}

/** collapse the given tiledob into the texture channel 1 of tarobj */
void collapseMapTiledTextures(ob_t * tarobj, ob_t * tiledob);
/** collapse the given skidsob into the texture channel 2 of tarobj */
void collapseSkidsGrassTextures(ob_t * tarobj, ob_t * skidsob);
/** collapse the given shadob into the texture channel 3 of tarobj */
void collapseShadowTextures(ob_t * tarobj, ob_t * shadob);

/** Match textures from ob1, ob2 and ob3 with ob0. In case a match is found
 *  add them as additional texture channels in ob0.
 */
void collapseTextures(ob_t * ob0, ob_t * ob1, ob_t * ob2, ob_t * ob3)
{
    ob_t * tmpob = NULL;

    tmpob = ob0;
    while (tmpob != NULL)
    {
        if (0 == isNamedAndPolygon(tmpob))
        {
            tmpob = tmpob->next;
            continue;
        }

        collapseMapTiledTextures(tmpob, ob1);
        collapseSkidsGrassTextures(tmpob, ob2);
        collapseShadowTextures(tmpob, ob3);

        tmpob = tmpob->next;
    }
}

/** copy the texture, textarray and vertexarray properties of srcob
 *  into the corresponding places in destob based on the given channel.
 *  The channel may be 1,2 or 3.
 */
void copyTextureChannel(ob_t * destob, ob_t * srcob, int channel)
{
    char* tex = srcob->texture;
    double* texarr = srcob->textarray;
    tcoord_t* vertarr = srcob->vertexarray;

    if (channel == 1)
    {
        destob->texture1 = tex;
        destob->textarray1 = texarr;
        destob->vertexarray1 = vertarr;
    }
    else if (channel == 2)
    {
        destob->texture2 = tex;
        destob->textarray2 = texarr;
        destob->vertexarray2 = vertarr;
    }
    else if (channel == 3)
    {
        destob->texture3 = tex;
        destob->textarray3 = texarr;
        destob->vertexarray3 = vertarr;
    }
}

void collapseMapTiledTextures(ob_t * tarob, ob_t * tiledob)
{
    ob_t * curtiledob = tiledob;
    int notinsameorder = FALSE;
    int curvert = 0;

    while (curtiledob != NULL)
    {
        if (0 == isNamedAndPolygon(curtiledob))
        {
            curtiledob = curtiledob->next;
            continue;
        }
        notinsameorder = FALSE;
        if (!stricmp(curtiledob->name, tarob->name)
        && tarob->numvert == curtiledob->numvert)
        {
            /* found an ob in ob1 */
            copyTextureChannel(tarob, curtiledob, 1);
            for (curvert = 0; curvert < tarob->numvert; curvert++)
            {
                if (fabs(tarob->vertex[curvert].x - curtiledob->vertex[curvert].x) > MINVAL
                || fabs(tarob->vertex[curvert].y - curtiledob->vertex[curvert].y)>MINVAL
                || fabs(tarob->vertex[curvert].z - curtiledob->vertex[curvert].z )>MINVAL)
                {
                    notinsameorder=TRUE;
                }
            }

            if (notinsameorder == TRUE)
            {
                printf(
                        "%s : points not in the same order, reordering ...\n",
                        tarob->name);
                reorder(tarob, curtiledob, tarob->textarray1,
                        tarob->vertexarray1);
                printf("%s : reordering ... done\n", tarob->name);
            }
            break;
        }
        curtiledob = curtiledob->next;
    }
}

void collapseSkidsGrassTextures(ob_t * tarob, ob_t * skidsob)
{
    ob_t * curskidsob = skidsob;

    while (curskidsob != NULL)
    {
        if (0 == isNamedAndPolygon(curskidsob))
        {
            curskidsob = curskidsob->next;
            continue;
        }

        if (!stricmp(curskidsob->name, tarob->name)
        && tarob->numvert == curskidsob->numvert)
        {
            /* found an ob in ob2 */
            copyTextureChannel(tarob, curskidsob, 2);
            break;
        }

        curskidsob = curskidsob->next;
    }
}

void collapseShadowTextures(ob_t * tarob, ob_t * shadob)
{
    ob_t * curshadob = shadob;
    int curvert = 0;

    while (curshadob != NULL)
    {
        if (0 == isNamedAndPolygon(curshadob))
        {
            curshadob = curshadob->next;
            continue;
        }

        if (!stricmp(curshadob->name, tarob->name)
        && tarob->numvert == curshadob->numvert)
        {
            /* found an ob in ob2 */
            copyTextureChannel(tarob, curshadob, 3);
            if (tarob->texture3)
            {
                for (curvert = 0; curvert < tarob->numvert; curvert++)
                {
                    if (tarob->textarray3[curvert * 2] != tarob->textarray[curvert * 2]
                    || tarob->textarray3[curvert * 2 + 1] != tarob->textarray[curvert * 2 + 1])
                    {
                        printf("name=%s %.2lf!=%.2lf %.2lf!=%.2lf\n",
                                tarob->name, tarob->textarray[curvert * 2],
                                tarob->textarray3[curvert * 2],
                                tarob->textarray[curvert * 2 + 1],
                                tarob->textarray3[curvert * 2 + 1]);
                    }
                }
            }

            break;
        }
        curshadob = curshadob->next;
    }
}
