#include<osg/Geometry>

#include <portability.h>

#include "OsgBrake.h"
#include "OsgUtil/OsgMath.h"

void SDBrakes::setCar(tCarElt * car)
{
    this->car = car;
}

osg::ref_ptr<osg::Geode> SDBrakes::initBrake(int wheelIndex)
{
    float	alpha;
    osg::Vec3 vtx;
    osg::Vec4 clr;
    osg::Vec3 nrm;
    osg::Vec2 tex;
    float	b_offset = 0;
    tdble	curAngle = 0.0;
    int i;

    osg::ref_ptr<osg::Geode> pBrake = new osg::Geode;

    pBrake->setName("Brake Assembly");
    // pWheel->addChild(pBrake);

    //grCarInfo[m_CarIndex].wheelselectorO[wheel_index] = pWheelSwitch;

    int brakeBranch = 32;
    float brakeAngle = 2.0 * SD_PI / (tdble)brakeBranch;
    float brakeOffset = 0.1;

    switch(wheelIndex)
    {
		case FRNT_RGT:
			curAngle = -(SD_PI / 2.0 + brakeAngle);
			b_offset = brakeOffset - car->_tireWidth(wheelIndex) / 2.0;
			break;
        case FRNT_LFT:
            curAngle = -(SD_PI / 2.0 + brakeAngle);
            b_offset = car->_tireWidth(wheelIndex) / 2.0 - brakeOffset;
            break;
        case REAR_RGT:
            curAngle = (SD_PI / 2.0 - brakeAngle);
            b_offset = brakeOffset - car->_tireWidth(wheelIndex) / 2.0;
            break;
        case REAR_LFT:
            curAngle = (SD_PI / 2.0 - brakeAngle);
            b_offset = car->_tireWidth(wheelIndex) / 2.0 - brakeOffset;
            break;
        }

        /* hub */
        osg::Vec3Array *hub_vtx = new osg::Vec3Array();
        osg::Vec4Array *hub_clr = new osg::Vec4Array();
        osg::Vec3Array *hub_nrm = new osg::Vec3Array();

        tdble hubRadius;

        /* center */
        vtx[0] = vtx[2] = 0.0;
        vtx[1] = b_offset;
        hub_vtx->push_back(vtx);

        hubRadius = car->_brakeDiskRadius(wheelIndex) * 0.6;
        for (i = 0; i < brakeBranch; i++)
        {
            alpha = (float)i * 2.0 * SD_PI / (float)(brakeBranch - 1);
            vtx[0] = hubRadius * cos(alpha);
            vtx[1] = b_offset;
            vtx[2] = hubRadius * sin(alpha);
            hub_vtx->push_back(vtx);
        }

        clr[0] = clr[1] = clr[2] = 0.0;
        clr[3] = 1.0;
        hub_clr->push_back(clr);
        nrm[0] = nrm[2] = 0.0;

        // Make normal point outside to have proper lighting.
        switch(wheelIndex)
        {
            case FRNT_RGT:
            case REAR_RGT:
                nrm[1] = -1.0;
                break;
            case FRNT_LFT:
            case REAR_LFT:
                nrm[1] = 1.0;
                break;
        }

        hub_nrm->push_back(nrm);

        osg::ref_ptr<osg::Geometry> pHub = new osg::Geometry;
        pHub->setName("Hub");
        pHub->addPrimitiveSet( new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_FAN, 0, brakeBranch+1 ));
        pHub->setVertexArray(hub_vtx);
        pHub->setNormalArray(hub_nrm);
        pHub->setColorArray(hub_clr);
        pHub->setNormalBinding(osg::Geometry::BIND_OVERALL);
        pHub->setNormalArray(hub_nrm);
        pHub->setColorBinding(osg::Geometry::BIND_OVERALL);
        pHub->setColorArray(hub_clr);

        pBrake->addDrawable(pHub);

        /* Brake disk */
        osg::Vec3Array *brk_vtx = new osg::Vec3Array();
        osg::Vec4Array *brk_clr = new osg::Vec4Array();
        osg::Vec3Array *brk_nrm = new osg::Vec3Array();

        for (i = 0; i < (brakeBranch / 2 + 2); i++)
        {
            alpha = curAngle + (float)i * 2.0 * SD_PI / (float)(brakeBranch - 1);
            vtx[0] = car->_brakeDiskRadius(wheelIndex) * cos(alpha);
            vtx[1] = b_offset;
            vtx[2] = car->_brakeDiskRadius(wheelIndex) * sin(alpha);
            brk_vtx->push_back(vtx);
            vtx[0] = car->_brakeDiskRadius(wheelIndex) * cos(alpha) * 0.6;
            vtx[1] = b_offset;
            vtx[2] = car->_brakeDiskRadius(wheelIndex) * sin(alpha) * 0.6;
            brk_vtx->push_back(vtx);
        }

        clr[0] = clr[1] = clr[2] = 0.1;
        clr[3] = 1.0;
        brk_clr->push_back(clr);
        brk_nrm->push_back(nrm);

        osg::ref_ptr<osg::Geometry> pBDisc = new osg::Geometry;
        pBDisc->setName("Disc Brake");

        pBDisc->addPrimitiveSet( new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, 0, brakeBranch+4 ));
        pBDisc->setVertexArray(brk_vtx);
        pBDisc->setNormalArray(brk_nrm);
        pBDisc->setColorArray(brk_clr);
        pBDisc->setNormalBinding(osg::Geometry::BIND_OVERALL);
        pBDisc->setNormalArray(brk_nrm);
        pBDisc->setColorBinding(osg::Geometry::BIND_OVERALL);
        pBDisc->setColorArray(brk_clr);

		//brkColorO[wheelIndex] = brk_clr;

        pBrake->addDrawable(pBDisc);

        this->brake_disks[wheelIndex] = pBDisc;

        /* Brake caliper */
        osg::Vec3Array *cal_vtx = new osg::Vec3Array();
        osg::Vec4Array *cal_clr = new osg::Vec4Array();
        osg::Vec3Array *cal_nrm = new osg::Vec3Array();

        for (i = 0; i < (brakeBranch / 2 - 2); i++)
        {
            alpha = - curAngle + (float)i * 2.0 * SD_PI / (float)(brakeBranch - 1);
            vtx[0] = (car->_brakeDiskRadius(wheelIndex) + 0.02) * cos(alpha);
            vtx[1] = b_offset;
            vtx[2] = (car->_brakeDiskRadius(wheelIndex) + 0.02) * sin(alpha);
            cal_vtx->push_back(vtx);
            vtx[0] = car->_brakeDiskRadius(wheelIndex) * cos(alpha) * 0.6;
            vtx[1] = b_offset;
            vtx[2] = car->_brakeDiskRadius(wheelIndex) * sin(alpha) * 0.6;
            cal_vtx->push_back(vtx);
        }

        clr[0] = 0.2;
        clr[1] = 0.2;
        clr[2] = 0.2;
        clr[3] = 1.0;
        cal_clr->push_back(clr);
        cal_nrm->push_back(nrm);

        osg::ref_ptr<osg::Geometry> pCaliper = new osg::Geometry;
        pCaliper->setName("Caliper");
        pCaliper->addPrimitiveSet( new osg::DrawArrays(osg::PrimitiveSet::TRIANGLE_STRIP, 0, brakeBranch-4 ));
        pCaliper->setVertexArray(cal_vtx);
        pCaliper->setNormalArray(cal_nrm);
        pCaliper->setColorArray(cal_clr);
        pCaliper->setNormalBinding(osg::Geometry::BIND_OVERALL);
        pCaliper->setNormalArray(cal_nrm);
        pCaliper->setColorBinding(osg::Geometry::BIND_OVERALL);
        pCaliper->setColorArray(cal_clr);

        pBrake->addDrawable(pCaliper);

       // this->brakes[wheelIndex] = pBrake;

        return pBrake;
}

void SDBrakes::updateBrakes()
{ // TODO clean unused memory ... free pointers if needed
    for(int i=0;i<4;i++)
	{
        osg::Vec4 clr;
        osg::Vec4Array *cal_clr = new osg::Vec4Array();

        clr[0] = 0.1 + car->_brakeTemp(i) * 1.5;
        clr[1] = 0.1 + car->_brakeTemp(i) * 0.3;
        clr[2] = 0.1 - car->_brakeTemp(i) * 0.3;
        clr[3] = 1.0;
        cal_clr->push_back(clr);

       this->brake_disks[i]->setColorArray(cal_clr);
    }
}
