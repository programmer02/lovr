#include "headset/headset.h"
#include "resources/actions.json.h"
#include "resources/bindings_vive.json.h"
#include "resources/bindings_knuckles.json.h"
#include "resources/bindings_touch.json.h"
#include "event/event.h"
#include "filesystem/filesystem.h"
#include "graphics/graphics.h"
#include "graphics/canvas.h"
#include "core/maf.h"
#include "core/ref.h"
#include "platform.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifndef WIN32
#pragma pack(push, 4)
#else
#undef EXTERN_C
#endif
#include <openvr_capi.h>
#ifndef WIN32
#pragma pack(pop)
#endif

// From openvr_capi.h
extern intptr_t VR_InitInternal(EVRInitError *peError, EVRApplicationType eType);
extern void VR_ShutdownInternal();
extern bool VR_IsHmdPresent();
extern intptr_t VR_GetGenericInterface(const char* pchInterfaceVersion, EVRInitError* peError);
extern bool VR_IsRuntimeInstalled();

#define HEADSET k_unTrackedDeviceIndex_Hmd
#define INVALID_DEVICE k_unTrackedDeviceIndexInvalid

static struct {
  struct VR_IVRSystem_FnTable* system;
  struct VR_IVRCompositor_FnTable* compositor;
  struct VR_IVRChaperone_FnTable* chaperone;
  struct VR_IVRRenderModels_FnTable* renderModels;
  struct VR_IVRInput_FnTable* input;
  VRActionSetHandle_t actionSet;
  VRActionHandle_t poseActions[MAX_DEVICES];
  VRActionHandle_t buttonActions[2][MAX_BUTTONS];
  VRActionHandle_t touchActions[2][MAX_BUTTONS];
  VRActionHandle_t axisActions[2][MAX_AXES];
  VRActionHandle_t skeletonActions[2];
  VRActionHandle_t hapticActions[2];
  TrackedDevicePose_t headPose;
  RenderModel_t* deviceModels[16];
  RenderModel_TextureMap_t* deviceTextures[16];
  Canvas* canvas;
  float* mask;
  float boundsGeometry[16];
  float clipNear;
  float clipFar;
  float offset;
  int msaa;
} state;

static TrackedDeviceIndex_t getDeviceIndex(Device device) {
  switch (device) {
    case DEVICE_HEAD: return HEADSET;
    case DEVICE_HAND_LEFT: return state.system->GetTrackedDeviceIndexForControllerRole(ETrackedControllerRole_TrackedControllerRole_LeftHand);
    case DEVICE_HAND_RIGHT: return state.system->GetTrackedDeviceIndexForControllerRole(ETrackedControllerRole_TrackedControllerRole_RightHand);
    default: return INVALID_DEVICE;
  }
}

static bool openvr_getName(char* name, size_t length);
static bool openvr_init(float offset, uint32_t msaa) {
  if (!VR_IsHmdPresent() || !VR_IsRuntimeInstalled()) {
    return false;
  }

  EVRInitError vrError;
  VR_InitInternal(&vrError, EVRApplicationType_VRApplication_Scene);
  if (vrError != EVRInitError_VRInitError_None) {
    return false;
  }

  char buffer[64];
  sprintf(buffer, "FnTable:%s", IVRSystem_Version), state.system = (struct VR_IVRSystem_FnTable*) VR_GetGenericInterface(buffer, &vrError);
  sprintf(buffer, "FnTable:%s", IVRCompositor_Version), state.compositor = (struct VR_IVRCompositor_FnTable*) VR_GetGenericInterface(buffer, &vrError);
  sprintf(buffer, "FnTable:%s", IVRChaperone_Version), state.chaperone = (struct VR_IVRChaperone_FnTable*) VR_GetGenericInterface(buffer, &vrError);
  sprintf(buffer, "FnTable:%s", IVRRenderModels_Version), state.renderModels = (struct VR_IVRRenderModels_FnTable*) VR_GetGenericInterface(buffer, &vrError);
  sprintf(buffer, "FnTable:%s", IVRInput_Version), state.input = (struct VR_IVRInput_FnTable*) VR_GetGenericInterface(buffer, &vrError);

  if (!state.system || !state.compositor || !state.chaperone || !state.renderModels || !state.input) {
    VR_ShutdownInternal();
    return false;
  }

  // Find the location of the action manifest, create it if it doesn't exist or isn't in the save directory
  const char* actionManifestLocation = lovrFilesystemGetRealDirectory("actions.json");
  if (!actionManifestLocation || strcmp(actionManifestLocation, lovrFilesystemGetSaveDirectory())) {
    if (
      lovrFilesystemWrite("actions.json", (const char*) actions_json, actions_json_len, false) != actions_json_len ||
      lovrFilesystemWrite("bindings_vive.json", (const char*) bindings_vive_json, bindings_vive_json_len, false) != bindings_vive_json_len ||
      lovrFilesystemWrite("bindings_knuckles.json", (const char*) bindings_knuckles_json, bindings_knuckles_json_len, false) != bindings_knuckles_json_len ||
      lovrFilesystemWrite("bindings_touch.json", (const char*) bindings_touch_json, bindings_touch_json_len, false) != bindings_touch_json_len
    ) {
      VR_ShutdownInternal();
      return false;
    }
  }

  char path[LOVR_PATH_MAX];
  snprintf(path, sizeof(path), "%s%cactions.json", lovrFilesystemGetSaveDirectory(), lovrDirSep);
  state.input->SetActionManifestPath(path);
  state.input->GetActionSetHandle("/actions/lovr", &state.actionSet);

  state.input->GetActionHandle("/actions/lovr/in/headPose", &state.poseActions[DEVICE_HEAD]);
  state.input->GetActionHandle("/actions/lovr/in/leftHandPose", &state.poseActions[DEVICE_HAND_LEFT]);
  state.input->GetActionHandle("/actions/lovr/in/rightHandPose", &state.poseActions[DEVICE_HAND_RIGHT]);

  state.input->GetActionHandle("/actions/lovr/in/leftTriggerDown", &state.buttonActions[0][BUTTON_TRIGGER]);
  state.input->GetActionHandle("/actions/lovr/in/leftThumbstickDown", &state.buttonActions[0][BUTTON_THUMBSTICK]);
  state.input->GetActionHandle("/actions/lovr/in/leftTouchpadDown", &state.buttonActions[0][BUTTON_TOUCHPAD]);
  state.input->GetActionHandle("/actions/lovr/in/leftGripDown", &state.buttonActions[0][BUTTON_GRIP]);
  state.input->GetActionHandle("/actions/lovr/in/leftMenuDown", &state.buttonActions[0][BUTTON_MENU]);
  state.input->GetActionHandle("/actions/lovr/in/leftADown", &state.buttonActions[0][BUTTON_A]);
  state.input->GetActionHandle("/actions/lovr/in/leftBDown", &state.buttonActions[0][BUTTON_B]);
  state.input->GetActionHandle("/actions/lovr/in/leftXDown", &state.buttonActions[0][BUTTON_X]);
  state.input->GetActionHandle("/actions/lovr/in/leftYDown", &state.buttonActions[0][BUTTON_Y]);

  state.input->GetActionHandle("/actions/lovr/in/rightTriggerDown", &state.buttonActions[1][BUTTON_TRIGGER]);
  state.input->GetActionHandle("/actions/lovr/in/rightThumbstickDown", &state.buttonActions[1][BUTTON_THUMBSTICK]);
  state.input->GetActionHandle("/actions/lovr/in/rightTouchpadDown", &state.buttonActions[1][BUTTON_TOUCHPAD]);
  state.input->GetActionHandle("/actions/lovr/in/rightGripDown", &state.buttonActions[1][BUTTON_GRIP]);
  state.input->GetActionHandle("/actions/lovr/in/rightMenuDown", &state.buttonActions[1][BUTTON_MENU]);
  state.input->GetActionHandle("/actions/lovr/in/rightADown", &state.buttonActions[1][BUTTON_A]);
  state.input->GetActionHandle("/actions/lovr/in/rightBDown", &state.buttonActions[1][BUTTON_B]);
  state.input->GetActionHandle("/actions/lovr/in/rightXDown", &state.buttonActions[1][BUTTON_X]);
  state.input->GetActionHandle("/actions/lovr/in/rightYDown", &state.buttonActions[1][BUTTON_Y]);

  state.input->GetActionHandle("/actions/lovr/in/leftTriggerTouch", &state.touchActions[0][BUTTON_TRIGGER]);
  state.input->GetActionHandle("/actions/lovr/in/leftThumbstickTouch", &state.touchActions[0][BUTTON_THUMBSTICK]);
  state.input->GetActionHandle("/actions/lovr/in/leftTouchpadTouch", &state.touchActions[0][BUTTON_TOUCHPAD]);
  state.input->GetActionHandle("/actions/lovr/in/leftGripTouch", &state.touchActions[0][BUTTON_GRIP]);
  state.input->GetActionHandle("/actions/lovr/in/leftMenuTouch", &state.touchActions[0][BUTTON_MENU]);
  state.input->GetActionHandle("/actions/lovr/in/leftATouch", &state.touchActions[0][BUTTON_A]);
  state.input->GetActionHandle("/actions/lovr/in/leftBTouch", &state.touchActions[0][BUTTON_B]);
  state.input->GetActionHandle("/actions/lovr/in/leftXTouch", &state.touchActions[0][BUTTON_X]);
  state.input->GetActionHandle("/actions/lovr/in/leftYTouch", &state.touchActions[0][BUTTON_Y]);

  state.input->GetActionHandle("/actions/lovr/in/rightTriggerTouch", &state.touchActions[1][BUTTON_TRIGGER]);
  state.input->GetActionHandle("/actions/lovr/in/rightThumbstickTouch", &state.touchActions[1][BUTTON_THUMBSTICK]);
  state.input->GetActionHandle("/actions/lovr/in/rightTouchpadTouch", &state.touchActions[1][BUTTON_TOUCHPAD]);
  state.input->GetActionHandle("/actions/lovr/in/rightGripTouch", &state.touchActions[1][BUTTON_GRIP]);
  state.input->GetActionHandle("/actions/lovr/in/rightMenuTouch", &state.touchActions[1][BUTTON_MENU]);
  state.input->GetActionHandle("/actions/lovr/in/rightATouch", &state.touchActions[1][BUTTON_A]);
  state.input->GetActionHandle("/actions/lovr/in/rightBTouch", &state.touchActions[1][BUTTON_B]);
  state.input->GetActionHandle("/actions/lovr/in/rightXTouch", &state.touchActions[1][BUTTON_X]);
  state.input->GetActionHandle("/actions/lovr/in/rightYTouch", &state.touchActions[1][BUTTON_Y]);

  state.input->GetActionHandle("/actions/lovr/in/leftTriggerAxis", &state.axisActions[0][AXIS_TRIGGER]);
  state.input->GetActionHandle("/actions/lovr/in/leftThumbstickAxis", &state.axisActions[0][AXIS_THUMBSTICK]);
  state.input->GetActionHandle("/actions/lovr/in/leftTouchpadAxis", &state.axisActions[0][AXIS_TOUCHPAD]);
  state.input->GetActionHandle("/actions/lovr/in/leftGripAxis", &state.axisActions[0][AXIS_GRIP]);

  state.input->GetActionHandle("/actions/lovr/in/rightTriggerAxis", &state.axisActions[1][AXIS_TRIGGER]);
  state.input->GetActionHandle("/actions/lovr/in/rightThumbstickAxis", &state.axisActions[1][AXIS_THUMBSTICK]);
  state.input->GetActionHandle("/actions/lovr/in/rightTouchpadAxis", &state.axisActions[1][AXIS_TOUCHPAD]);
  state.input->GetActionHandle("/actions/lovr/in/rightGripAxis", &state.axisActions[1][AXIS_GRIP]);

  state.input->GetActionHandle("/actions/lovr/in/leftHandSkeleton", &state.skeletonActions[0]);
  state.input->GetActionHandle("/actions/lovr/in/rightHandSkeleton", &state.skeletonActions[1]);

  state.input->GetActionHandle("/actions/lovr/out/leftHandBZZ", &state.hapticActions[0]);
  state.input->GetActionHandle("/actions/lovr/out/rightHandBZZ", &state.hapticActions[1]);

  state.clipNear = 0.1f;
  state.clipFar = 30.f;
  state.offset = state.compositor->GetTrackingSpace() == ETrackingUniverseOrigin_TrackingUniverseStanding ? 0. : offset;
  state.msaa = msaa;

  return true;
}

static void openvr_destroy(void) {
  lovrRelease(Canvas, state.canvas);
  for (int i = 0; i < 16; i++) {
    if (state.deviceModels[i]) {
      state.renderModels->FreeRenderModel(state.deviceModels[i]);
    }
    if (state.deviceTextures[i]) {
      state.renderModels->FreeTexture(state.deviceTextures[i]);
    }
    state.deviceModels[i] = NULL;
    state.deviceTextures[i] = NULL;
  }
  VR_ShutdownInternal();
  free(state.mask);
  memset(&state, 0, sizeof(state));
}

static bool openvr_getName(char* name, size_t length) {
  ETrackedPropertyError error;
  state.system->GetStringTrackedDeviceProperty(HEADSET, ETrackedDeviceProperty_Prop_ManufacturerName_String, name, (uint32_t) length, &error);
  return error == ETrackedPropertyError_TrackedProp_Success;
}

static HeadsetOrigin openvr_getOriginType(void) {
  switch (state.compositor->GetTrackingSpace()) {
    case ETrackingUniverseOrigin_TrackingUniverseSeated: return ORIGIN_HEAD;
    case ETrackingUniverseOrigin_TrackingUniverseStanding: return ORIGIN_FLOOR;
    default: return ORIGIN_HEAD;
  }
}

static void openvr_getDisplayDimensions(uint32_t* width, uint32_t* height) {
  state.system->GetRecommendedRenderTargetSize(width, height);
}

static const float* openvr_getDisplayMask(uint32_t* count) {
  struct HiddenAreaMesh_t hiddenAreaMesh = state.system->GetHiddenAreaMesh(EVREye_Eye_Left, EHiddenAreaMeshType_k_eHiddenAreaMesh_Standard);

  if (hiddenAreaMesh.unTriangleCount == 0) {
    *count = 0;
    return NULL;
  }

  state.mask = realloc(state.mask, hiddenAreaMesh.unTriangleCount * 3 * 2 * sizeof(float));
  lovrAssert(state.mask, "Out of memory");

  for (uint32_t i = 0; i < 3 * hiddenAreaMesh.unTriangleCount; i++) {
    state.mask[2 * i + 0] = hiddenAreaMesh.pVertexData[i].v[0];
    state.mask[2 * i + 1] = hiddenAreaMesh.pVertexData[i].v[1];
  }

  *count = hiddenAreaMesh.unTriangleCount * 3 * 2;
  return state.mask;
}

static double openvr_getDisplayTime(void) {
  float secondsSinceVsync;
  state.system->GetTimeSinceLastVsync(&secondsSinceVsync, NULL);

  float frequency = state.system->GetFloatTrackedDeviceProperty(HEADSET, ETrackedDeviceProperty_Prop_DisplayFrequency_Float, NULL);
  float frameDuration = 1.f / frequency;
  float vsyncToPhotons = state.system->GetFloatTrackedDeviceProperty(HEADSET, ETrackedDeviceProperty_Prop_SecondsFromVsyncToPhotons_Float, NULL);

  return lovrPlatformGetTime() + (double) (frameDuration - secondsSinceVsync + vsyncToPhotons);
}

static void openvr_getClipDistance(float* clipNear, float* clipFar) {
  *clipNear = state.clipNear;
  *clipFar = state.clipFar;
}

static void openvr_setClipDistance(float clipNear, float clipFar) {
  state.clipNear = clipNear;
  state.clipFar = clipFar;
}

static void openvr_getBoundsDimensions(float* width, float* depth) {
  state.chaperone->GetPlayAreaSize(width, depth);
}

static const float* openvr_getBoundsGeometry(uint32_t* count) {
  struct HmdQuad_t quad;
  if (state.chaperone->GetPlayAreaRect(&quad)) {
    for (int i = 0; i < 4; i++) {
      state.boundsGeometry[4 * i + 0] = quad.vCorners[i].v[0];
      state.boundsGeometry[4 * i + 1] = quad.vCorners[i].v[1];
      state.boundsGeometry[4 * i + 2] = quad.vCorners[i].v[2];
    }

    *count = 16;
    return state.boundsGeometry;
  }

  return NULL;
}

static bool openvr_getPose(Device device, vec3 position, quat orientation) {
  InputPoseActionData_t actionData;
  TrackedDevicePose_t* pose;

  if (device == DEVICE_HEAD) {
    pose = &state.headPose;
  } else if (device == DEVICE_HAND_LEFT || device == DEVICE_HAND_RIGHT) {
    state.input->GetPoseActionData(state.poseActions[device], state.compositor->GetTrackingSpace(), 0.f, &actionData, sizeof(actionData), 0);
    pose = &actionData.pose;
  } else {
    return false;
  }

  float transform[16];
  mat4_fromMat34(transform, pose->mDeviceToAbsoluteTracking.m);
  mat4_getPosition(transform, position);
  mat4_getOrientation(transform, orientation);
  transform[13] += state.offset;
  return pose->bPoseIsValid;
}

static bool openvr_getVelocity(Device device, vec3 velocity, vec3 angularVelocity) {
  InputPoseActionData_t actionData;
  TrackedDevicePose_t* pose;

  if (device == DEVICE_HEAD) {
    pose = &state.headPose;
  } else if (device == DEVICE_HAND_LEFT || device == DEVICE_HAND_RIGHT) {
    state.input->GetPoseActionData(state.poseActions[device], state.compositor->GetTrackingSpace(), 0.f, &actionData, sizeof(actionData), 0);
    pose = &actionData.pose;
  } else {
    return false;
  }

  vec3_init(velocity, pose->vVelocity.v);
  vec3_init(angularVelocity, pose->vAngularVelocity.v);
  return pose->bPoseIsValid;
}

static bool getButtonState(Device device, DeviceButton button, VRActionHandle_t actions[2][MAX_BUTTONS], bool* value) {
  if (device != DEVICE_HAND_LEFT && device != DEVICE_HAND_RIGHT) {
    return false;
  }

  InputDigitalActionData_t actionData;
  state.input->GetDigitalActionData(actions[device - DEVICE_HAND_LEFT][button], &actionData, sizeof(actionData), 0);
  *value = actionData.bState;
  return actionData.bActive;
}

static bool openvr_isDown(Device device, DeviceButton button, bool* down) {
  return getButtonState(device, button, state.buttonActions, down);
}

static bool openvr_isTouched(Device device, DeviceButton button, bool* touched) {
  return getButtonState(device, button, state.touchActions, touched);
}

static bool openvr_getAxis(Device device, DeviceAxis axis, vec3 value) {
  if (device != DEVICE_HAND_LEFT && device != DEVICE_HAND_RIGHT) {
    return false;
  }

  InputAnalogActionData_t actionData;
  state.input->GetAnalogActionData(state.axisActions[device - DEVICE_HAND_LEFT][axis], &actionData, sizeof(actionData), 0);
  vec3_set(value, actionData.x, actionData.y, actionData.z);
  return actionData.bActive;
}

static bool openvr_vibrate(Device device, float strength, float duration, float frequency) {
  if (duration <= 0.f || (device != DEVICE_HAND_LEFT && device != DEVICE_HAND_RIGHT)) return false;

  if (frequency <= 0.f) {
    frequency = 1.f;
  }

  state.input->TriggerHapticVibrationAction(state.hapticActions[device - DEVICE_HAND_LEFT], 0.f, duration, frequency, strength, 0);
  return true;
}

static ModelData* openvr_newModelData(Device device) {
  TrackedDeviceIndex_t index = getDeviceIndex(device);
  if (index == INVALID_DEVICE) return false;

  char renderModelName[1024];
  ETrackedDeviceProperty renderModelNameProperty = ETrackedDeviceProperty_Prop_RenderModelName_String;
  state.system->GetStringTrackedDeviceProperty(index, renderModelNameProperty, renderModelName, 1024, NULL);

  if (!state.deviceModels[index]) {
    while (state.renderModels->LoadRenderModel_Async(renderModelName, &state.deviceModels[index]) == EVRRenderModelError_VRRenderModelError_Loading) {
      lovrPlatformSleep(.001);
    }
  }

  if (!state.deviceTextures[index]) {
    while (state.renderModels->LoadTexture_Async(state.deviceModels[index]->diffuseTextureId, &state.deviceTextures[index]) == EVRRenderModelError_VRRenderModelError_Loading) {
      lovrPlatformSleep(.001);
    }
  }

  RenderModel_t* vrModel = state.deviceModels[index];
  ModelData* model = lovrAlloc(ModelData);
  size_t vertexSize = sizeof(RenderModel_Vertex_t);

  model->bufferCount = 2;
  model->attributeCount = 4;
  model->textureCount = 1;
  model->materialCount = 1;
  model->primitiveCount = 1;
  model->nodeCount = 1;
  lovrModelDataAllocate(model);

  model->buffers[0] = (ModelBuffer) {
    .data = (char*) vrModel->rVertexData,
    .size = vrModel->unVertexCount * vertexSize,
    .stride = vertexSize
  };

  model->buffers[1] = (ModelBuffer) {
    .data = (char*) vrModel->rIndexData,
    .size = vrModel->unTriangleCount * 3 * sizeof(uint16_t),
    .stride = sizeof(uint16_t)
  };

  model->attributes[0] = (ModelAttribute) {
    .buffer = 0,
    .offset = offsetof(RenderModel_Vertex_t, vPosition),
    .count = vrModel->unVertexCount,
    .type = F32,
    .components = 3
  };

  model->attributes[1] = (ModelAttribute) {
    .buffer = 0,
    .offset = offsetof(RenderModel_Vertex_t, vNormal),
    .count = vrModel->unVertexCount,
    .type = F32,
    .components = 3
  };

  model->attributes[2] = (ModelAttribute) {
    .buffer = 0,
    .offset = offsetof(RenderModel_Vertex_t, rfTextureCoord),
    .count = vrModel->unVertexCount,
    .type = F32,
    .components = 2
  };

  model->attributes[3] = (ModelAttribute) {
    .buffer = 1,
    .offset = 0,
    .count = vrModel->unTriangleCount * 3,
    .type = U16,
    .components = 1
  };

  RenderModel_TextureMap_t* vrTexture = state.deviceTextures[index];
  model->textures[0] = lovrTextureDataCreate(vrTexture->unWidth, vrTexture->unHeight, 0, FORMAT_RGBA);
  memcpy(model->textures[0]->blob.data, vrTexture->rubTextureMapData, vrTexture->unWidth * vrTexture->unHeight * 4);

  model->materials[0] = (ModelMaterial) {
    .colors[COLOR_DIFFUSE] = { 1.f, 1.f, 1.f, 1.f },
    .textures[TEXTURE_DIFFUSE] = 0,
    .filters[TEXTURE_DIFFUSE] = lovrGraphicsGetDefaultFilter()
  };

  model->primitives[0] = (ModelPrimitive) {
    .mode = DRAW_TRIANGLES,
    .attributes = {
      [ATTR_POSITION] = &model->attributes[0],
      [ATTR_NORMAL] = &model->attributes[1],
      [ATTR_TEXCOORD] = &model->attributes[2]
    },
    .indices = &model->attributes[3],
    .material = 0
  };

  model->nodes[0] = (ModelNode) {
    .transform = MAT4_IDENTITY,
    .primitiveIndex = 0,
    .primitiveCount = 1,
    .skin = ~0u,
    .matrix = true
  };

  return model;
}

static void openvr_renderTo(void (*callback)(void*), void* userdata) {
  if (!state.canvas) {
    uint32_t width, height;
    state.system->GetRecommendedRenderTargetSize(&width, &height);
    CanvasFlags flags = { .depth = { true, false, FORMAT_D24S8 }, .stereo = true, .mipmaps = true, .msaa = state.msaa };
    state.canvas = lovrCanvasCreate(width, height, flags);
    Texture* texture = lovrTextureCreate(TEXTURE_2D, NULL, 0, true, true, state.msaa);
    lovrTextureAllocate(texture, width * 2, height, 1, FORMAT_RGBA);
    lovrTextureSetFilter(texture, lovrGraphicsGetDefaultFilter());
    lovrCanvasSetAttachments(state.canvas, &(Attachment) { texture, 0, 0 }, 1);
    lovrRelease(Texture, texture);
    lovrPlatformSetSwapInterval(0);
  }

  Camera camera = { .canvas = state.canvas, .viewMatrix = { MAT4_IDENTITY, MAT4_IDENTITY } };

  float head[16], eye[16];
  mat4_fromMat34(head, state.headPose.mDeviceToAbsoluteTracking.m);

  for (int i = 0; i < 2; i++) {
    EVREye vrEye = (i == 0) ? EVREye_Eye_Left : EVREye_Eye_Right;
    mat4_fromMat44(camera.projection[i], state.system->GetProjectionMatrix(vrEye, state.clipNear, state.clipFar).m);
    mat4_multiply(camera.viewMatrix[i], head);
    mat4_multiply(camera.viewMatrix[i], mat4_fromMat34(eye, state.system->GetEyeToHeadTransform(vrEye).m));
    mat4_invertPose(camera.viewMatrix[i]);
  }

  lovrGraphicsSetCamera(&camera, true);
  callback(userdata);
  lovrGraphicsSetCamera(NULL, false);

  // Submit
  const Attachment* attachments = lovrCanvasGetAttachments(state.canvas, NULL);
  ptrdiff_t id = attachments[0].texture->id;
  Texture_t eyeTexture = { (void*) id, ETextureType_TextureType_OpenGL, EColorSpace_ColorSpace_Linear };
  VRTextureBounds_t left = { 0.f, 0.f, .5f, 1.f };
  VRTextureBounds_t right = { .5f, 0.f, 1.f, 1.f };
  state.compositor->Submit(EVREye_Eye_Left, &eyeTexture, &left, EVRSubmitFlags_Submit_Default);
  state.compositor->Submit(EVREye_Eye_Right, &eyeTexture, &right, EVRSubmitFlags_Submit_Default);
  lovrGpuDirtyTexture();
}

static Texture* openvr_getMirrorTexture(void) {
  return lovrCanvasGetAttachments(state.canvas, NULL)[0].texture;
}

static void openvr_update(float dt) {
  state.compositor->WaitGetPoses(&state.headPose, 1, NULL, 0);
  VRActiveActionSet_t activeActionSet = { .ulActionSet = state.actionSet };
  state.input->UpdateActionState(&activeActionSet, sizeof(activeActionSet), 1);

  struct VREvent_t vrEvent;
  while (state.system->PollNextEvent(&vrEvent, sizeof(vrEvent))) {
    switch (vrEvent.eventType) {
      case EVREventType_VREvent_InputFocusCaptured:
      case EVREventType_VREvent_InputFocusReleased: {
        bool isFocused = vrEvent.eventType == EVREventType_VREvent_InputFocusReleased;
        lovrEventPush((Event) { .type = EVENT_FOCUS, .data.boolean = { isFocused } });
        break;
      }

      default: break;
    }
  }
}

HeadsetInterface lovrHeadsetOpenVRDriver = {
  .driverType = DRIVER_OPENVR,
  .init = openvr_init,
  .destroy = openvr_destroy,
  .getName = openvr_getName,
  .getOriginType = openvr_getOriginType,
  .getDisplayDimensions = openvr_getDisplayDimensions,
  .getDisplayMask = openvr_getDisplayMask,
  .getDisplayTime = openvr_getDisplayTime,
  .getClipDistance = openvr_getClipDistance,
  .setClipDistance = openvr_setClipDistance,
  .getBoundsDimensions = openvr_getBoundsDimensions,
  .getBoundsGeometry = openvr_getBoundsGeometry,
  .getPose = openvr_getPose,
  .getVelocity = openvr_getVelocity,
  .isDown = openvr_isDown,
  .isTouched = openvr_isTouched,
  .getAxis = openvr_getAxis,
  .vibrate = openvr_vibrate,
  .newModelData = openvr_newModelData,
  .renderTo = openvr_renderTo,
  .getMirrorTexture = openvr_getMirrorTexture,
  .update = openvr_update
};
