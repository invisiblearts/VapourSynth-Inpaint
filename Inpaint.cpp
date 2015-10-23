#include "Inpaint.h"

static void InpaintMain(VSFrameRef *dstf, const VSFrameRef *srcf, const VSFrameRef *maskf, const InpaintData *d, VSFrameContext *frameCtx, const VSAPI *vsapi)
{
	const int numPlanes = d->fi_v->numPlanes;
	const int type = CV_8UC1;
	auto generateMat = [vsapi, d, type](const VSFrameRef *frmf, int plane = 0)->cv::Mat*
	{
		const int planeHeight = vsapi->getFrameHeight(frmf, plane), planeWidth = vsapi->getFrameWidth(frmf, plane);
		cv::Mat *mat = new cv::Mat(planeHeight, planeWidth, type, vsapi->getWritePtr(const_cast<VSFrameRef *>(frmf), plane));
		return mat;
	};
	if (numPlanes == 1)
	{
		cv::Mat *dstm = generateMat(dstf);
		cv::Mat *srcm = generateMat(srcf);
		cv::Mat *maskm = generateMat(maskf);
		cv::inpaint(*srcm, *maskm, *dstm, d->radius, d->method);
		delete dstm;
		delete srcm;
		delete maskm;
	}else if (numPlanes == 3){
		cv::Mat *srcm3[3], *dstm3[3], *maskm3[3];
		for (int i = 0; i < 3; ++i) {
			srcm3[i] = generateMat(srcf, i);
			maskm3[i] = generateMat(maskf, i);
			dstm3[i] = generateMat(dstf, i);
			cv::inpaint(*srcm3[i], *maskm3[i], *dstm3[i], d->radius, d->method);
			delete srcm3[i];
			delete maskm3[i];
			delete dstm3[i];
		}
	}
	return;
}


static void VS_CC InpaintCreate(const VSMap *in, VSMap *out, void *userData, VSCore *core, const VSAPI *vsapi)
{
    
    InpaintData *data = new InpaintData(vsapi);
    InpaintData &d = *data;
    
    int error;
    
    d.node_v = vsapi->propGetNode(in, "input", 0, nullptr);
    d.vi_v = vsapi->getVideoInfo(d.node_v);
    d.node_m = vsapi->propGetNode(in, "mask", 0, nullptr);
    d.vi_m = vsapi->getVideoInfo(d.node_m);
    
    if (!d.vi_v->format || !d.vi_m->format)
    {
        delete data;
        vsapi->setError(out, "inpaint.Inpaint: Invalid input clip. Only constant format input supported!");
        return;
    }
    
    if (d.vi_v->format->sampleType != stInteger || d.vi_v->format->bytesPerSample != 1)
    {
		// cv::inpaint only supports 8 bit, though its internal precision is 32 bit float.
		// Don't ask me why; ask Intel.
        delete data;
        vsapi->setError(out, "inpaint.Inpaint: Invalid input clip. Only 8 bit int format supported!");
        return;
    }
    
    if (strcmp(d.vi_m->format->name, d.vi_v->format->name) || d.vi_v->height != d.vi_m->height || d.vi_v->width != d.vi_m-> width || d.vi_v->numFrames != d.vi_m->numFrames){
        delete data;
        vsapi->setError(out, "inpaint.Inpaint: Invalid mask clip. Must be same as input!");
		return;
    }

    d.radius = int64ToIntS(vsapi->propGetInt(in, "radius", 0, &error));
    if (error)
        d.radius = 3;
    
    int i = int64ToIntS(vsapi->propGetInt(in, "method", 0, &error));
    
    if (error)
        d.method = METHOD_TELEA;
    
    if (i == METHOD_NS) {
        d.method = i;
    }
    else {
        d.method = METHOD_TELEA;
    }
    
    // Create filter
    vsapi->createFilter(in, out, "Inpaint", InpaintInit, InpaintGetFrame, InpaintFree, fmParallel, 0, data, core);
}

static void VS_CC InpaintInit(VSMap *in, VSMap *out, void **instanceData, VSNode *node, VSCore *core, const VSAPI *vsapi)
{
    InpaintData * d = static_cast<InpaintData *>(*instanceData);
    vsapi->setVideoInfo(d->vi_v, 1, node);
}

static const VSFrameRef *VS_CC InpaintGetFrame(int n, int activationReason, void **instanceData, void **frameData, VSFrameContext *frameCtx, VSCore *core, const VSAPI *vsapi)
{
    InpaintData * d = static_cast<InpaintData *>(*instanceData);
    if (activationReason == arInitial) {
        vsapi->requestFrameFilter(n, d->node_v, frameCtx);
        vsapi->requestFrameFilter(n, d->node_m, frameCtx);
    }
    else if (activationReason == arAllFramesReady){
		const VSFrameRef *src = vsapi->getFrameFilter(n, d->node_v, frameCtx);
		d->fi_v = vsapi->getFrameFormat(src);
		const VSFrameRef *mask = vsapi->getFrameFilter(n, d->node_m, frameCtx);
		const int width = vsapi->getFrameWidth(src, 0);
		const int height = vsapi->getFrameHeight(src, 0);
		VSFrameRef *dst = vsapi->newVideoFrame(d->fi_v, width, height, src, core);
		InpaintMain(dst, src, mask, d, frameCtx, vsapi);
		vsapi->freeFrame(src);
		vsapi->freeFrame(mask);
		return dst;
    }
    return nullptr;
}

static void VS_CC InpaintFree(void *instanceData, VSCore *core, const VSAPI *vsapi) {
	InpaintData * d = static_cast<InpaintData *>(instanceData);
	delete d;
}


VS_EXTERNAL_API(void) VapourSynthPluginInit(VSConfigPlugin configFunc, VSRegisterFunction registerFunc, VSPlugin *plugin)
{
    configFunc("jingjing.jia.wo", "inpaint",
               "Inpaint images using cv::inpaint.",
               VAPOURSYNTH_API_VERSION, 1, plugin);
    registerFunc("Inpaint", "input:clip;mask:clip;radius:int:opt;method:int:opt", InpaintCreate, nullptr, plugin);
}