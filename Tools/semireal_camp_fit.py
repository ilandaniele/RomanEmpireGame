"""Read-only building-envelope diagnostics. Not placement, nav or spawn authority."""
import math


def vector(value, size, label, *, positive=False):
    if not isinstance(value,(tuple,list)) or len(value)!=size or any(
            isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v) or
            (positive and v<=0) for v in value):
        raise ValueError('Invalid '+label)
    return tuple(float(v) for v in value)


def analyze_fit(bounds_min, bounds_max, footprint_grid, prototype_scale,
                spawn_offset, capsule_radius, anchor_half_height):
    """Compare an asset AABB with existing actor-local rules; mutate nothing.

    Assumptions bind to BuildingBase/Placement source in the native receipt:
    footprint units*100cm, placement anchor=collision box center. Clearance is
    conservative XY AABB math, NOT a wall/door/capsule overlap or nav query.
    """
    lo=vector(bounds_min,3,'bounds'); hi=vector(bounds_max,3,'bounds')
    footprint=vector(footprint_grid,2,'footprint',positive=True)
    scale=vector(prototype_scale,3,'scale',positive=True)
    spawn=vector(spawn_offset,3,'spawn')
    radius,height=vector([capsule_radius,anchor_half_height],2,'capsule/anchor',positive=True)
    size=tuple(b-a for a,b in zip(lo,hi))
    if any(v<=0 for v in size):
        raise ValueError('Invalid ordered bounds')
    center=tuple((a+b)/2 for a,b in zip(lo,hi))
    half=tuple(v/2 for v in size)
    footprint_cm=tuple(v*100 for v in footprint)
    qx,qy=abs(spawn[0])-half[0],abs(spawn[1])-half[1]
    point_distance=math.hypot(max(qx,0),max(qy,0))+min(max(qx,qy),0)
    collision_qx,collision_qy=abs(spawn[0])-footprint_cm[0]/2,abs(spawn[1])-footprint_cm[1]/2
    collision_distance=math.hypot(max(collision_qx,0),max(collision_qy,0))+min(max(collision_qx,collision_qy),0)
    return dict(
        source_size_cm=size,source_center_cm=center,
        natural_centered_footprint_fits=all(size[i]<=footprint_cm[i] for i in (0,1)),
        current_footprint_cm=footprint_cm,
        minimum_enclosing_footprint_grid=tuple(v/100 for v in size[:2]),
        enclosing_whole_grid_proposal=tuple(math.ceil(v/100) for v in size[:2]),
        uniform_scale_to_fit_current_xy=min(footprint_cm[i]/size[i] for i in (0,1)),
        natural_centered_grounded_relative_offset_cm=(-center[0],-center[1],-height-lo[2]),
        naive_swap_scaled_size_cm=tuple(v*s for v,s in zip(size,scale)),
        naive_swap_ground_min_z_cm=height+lo[2]*scale[2],
        natural_centered_spawn_capsule_xy_clearance_cm=point_distance-radius,
        current_collision_box_spawn_capsule_xy_clearance_cm=collision_distance-radius,
        natural_positive_x_spawn_center_min_cm=half[0]+radius,
        current_box_positive_x_spawn_center_min_cm=footprint_cm[0]/2+radius,
        limits='XY envelope only; no actual overlap/door/terrain/nav/PIE/player/adoption approval')
