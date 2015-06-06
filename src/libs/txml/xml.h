/*
 * xml.h -- Interface file for XML 
 *
 * @(#) $Id: xml.h 2917 2010-10-17 19:03:40Z pouillot $
 */
 
#ifndef _XML_H_
#define _XML_H_

// DLL exported symbols declarator for Windows.
#ifdef WIN32
# ifdef TXML_DLL
#  define TXML_API __declspec(dllexport)
# else
#  define TXML_API __declspec(dllimport)
# endif
#else
# define TXML_API
#endif


typedef struct xmlAttribute {
    char		*name;
    char		*value;
    struct xmlAttribute	*next;
} txmlAttribute;

typedef struct xmlElement {
    char		*name;		/* element name */
    char		*pcdata;	/* string associated with this element */
    struct xmlAttribute	*attr;		/* attributes of this element */
    int			level;		/* nested level */
    struct xmlElement	*next;		/* next element at the same level */
    struct xmlElement	*sub;		/* next element at the next level (nested) */
    struct xmlElement	*up;		/* upper element */
} txmlElement;    

TXML_API txmlElement *xmlInsertElt(txmlElement *curElt, const char *name, const char **atts);
TXML_API txmlElement *xmlReadFile(const char *file);
TXML_API int          xmlWriteFile(const char *file, txmlElement *startElt, char *dtd);
TXML_API char        *xmlGetAttr(txmlElement *curElt, char *attrname);
TXML_API txmlElement *xmlNextElt(txmlElement *startElt);
TXML_API txmlElement *xmlSubElt(txmlElement *startElt);
TXML_API txmlElement *xmlWalkElt(txmlElement *startElt);
TXML_API txmlElement *xmlWalkSubElt(txmlElement *startElt, txmlElement *topElt);
TXML_API txmlElement *xmlFindNextElt(txmlElement *startElt, char *name);
TXML_API txmlElement *xmlFindEltAttr(txmlElement *startElt, char *name, char *attrname, char *attrvalue);

#endif /* _XML_H_ */ 



