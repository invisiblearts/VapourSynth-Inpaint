#ifndef INPAINT_H
#define INPAINT_H

#include <vapoursynth.h>
#include <VSHelper.h>
#include <opencv2/opencv.hpp>

enum {
    METHOD_NS = 0,
    METHOD_TELEA = 1
};

class InpaintData {
public:
    const VSAPI *vsapi = nullptr;
    VSNodeRef *node_v = nullptr;
    const VSVideoInfo *vi_v = nullptr;
    VSNodeRef *node_m = nullptr;
    const VSVideoInfo *vi_m = nullptr;
	const VSFormat *fi_v = nullptr;
    int method;
    int radius;
private:
public:
    InpaintData(const VSAPI *_vsapi)
    : vsapi(_vsapi){}
    
    ~InpaintData()
    {
        if (node_v) vsapi->freeNode(node_v);
        if (node_m) vsapi->freeNode(node_m);
    }
    
};

static void InpaintMain(VSFrameRef *dstf, const VSFrameRef *srcf, const VSFrameRef *maskf, const InpaintData *d, VSFrameContext *frameCtx, const VSAPI *vsapi);
static void VS_CC InpaintCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi);
static void VS_CC InpaintInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi);
static const VSFrameRef *VS_CC InpaintGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi);
static void VS_CC InpaintFree(void *instanceData, VSCore *core, const VSAPI *vsapi);

#endif
