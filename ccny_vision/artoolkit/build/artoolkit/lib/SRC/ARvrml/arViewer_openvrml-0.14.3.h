// -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; -*-

#ifndef AR_VRMLINT_H
#define AR_VRMLINT_H

#include <openvrml/browser.h>
#include <openvrml/gl/viewer.h>
#include <openvrml/bounding_volume.h>

class arVrmlViewer : public openvrml::gl::viewer {

public:
  arVrmlViewer(openvrml::browser& browser);
  ~arVrmlViewer();

    char             filename[512];
    double           translation[3];
    double           rotation[4];
    double           scale[3];
    bool             internal_light;

    void timerUpdate();
    void redraw();
    void setInternalLight( bool f );

protected:
    virtual void post_redraw();
    virtual void set_cursor(openvrml::gl::viewer::cursor_style c);
    virtual void swap_buffers();
    virtual void set_timer(double);


   virtual void set_viewpoint(const openvrml::vec3f & position,
                                       const openvrml::rotation & orientation,
                                       float fieldOfView,
                                       float avatarSize,
                                       float visibilityLimit);

    virtual viewer::object_t insert_background(const std::vector<float> & groundAngle,
                              const std::vector<openvrml::color> & groundColor,
                              const std::vector<float> & skyAngle,
                              const std::vector<openvrml::color> & skyColor,
                              size_t * whc = 0,
                              unsigned char ** pixels = 0);

    virtual viewer::object_t insert_dir_light(float ambientIntensity,
                                              float intensity,
                                              const openvrml::color & color,
                                              const openvrml::vec3f & direction);

    virtual viewer::object_t insert_point_light(float ambientIntensity,
                                                const openvrml::vec3f & attenuation,
                                                const openvrml::color & color,
                                                float intensity,
                                                const openvrml::vec3f & location,
                                                float radius);

    virtual viewer::object_t insert_spot_light(float ambientIntensity,
                                               const openvrml::vec3f & attenuation,
                                               float beamWidth,
                                               const openvrml::color & color,
                                               float cutOffAngle,
                                               const openvrml::vec3f & direction,
                                               float intensity,
                                               const openvrml::vec3f & location,
                                               float radius);
    virtual openvrml::bounding_volume::intersection
      intersect_view_volume(const openvrml::bounding_volume & bvolume) const;
};

#endif
