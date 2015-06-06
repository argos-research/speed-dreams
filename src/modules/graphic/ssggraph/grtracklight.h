#ifndef MK_TRACKLIGHT_H
#define MK_TRACKLIGHT_H

typedef struct Situation tSituation;

void grTrackLightInit();
void grTrackLightUpdate( tSituation *s );
void grTrackLightShutdown();

#endif //MK_TRACKLIGHT_H

