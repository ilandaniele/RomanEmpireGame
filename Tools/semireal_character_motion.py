"""Source-art-only motion corrections; never moves gameplay actors or damage."""
import math
from roman_character_recipe import CLIPS, rotations as prototype_rotations


def rotations(clip,time):
    result=prototype_rotations(clip,time)
    if clip=="Block":
        t=time/CLIPS[clip]
        hold=max(0.,min(1.,t*5,(1-t)*5)); hold=hold*hold*(3-2*hold)
        # Raise the arm while counter-rotating the wrist, so the scutum guards
        # the torso/front instead of rotating overhead like a roof.
        result={"upper_arm_l":(-18*hold,0),"forearm_l":(-32*hold,0),"hand_l":(50*hold,0)}
    elif clip=="Walk":
        for side in ("l","r"):
            pitch=result[f"thigh_{side}"][0]+result[f"calf_{side}"][0]
            result[f"foot_{side}"]=(-pitch,0)
    return result


def root_vertical_offset(clip,time,layout):
    if clip!="Walk":
        return 0.
    positions={name:point for name,_,point in layout}
    angles=rotations(clip,time)

    def rotate(vector,degrees):
        x,y,z=vector; t=math.radians(degrees)
        return x,y*math.cos(t)-z*math.sin(t),y*math.sin(t)+z*math.cos(t)

    rises=[]
    for side in ("l","r"):
        hip,knee,ankle=[positions[f"{part}_{side}"] for part in ("thigh","calf","foot")]
        thigh=angles[f"thigh_{side}"][0]
        calf=angles[f"calf_{side}"][0]
        upper=rotate(tuple(b-a for a,b in zip(hip,knee)),thigh)
        lower=rotate(tuple(b-a for a,b in zip(knee,ankle)),thigh+calf)
        rises.append(hip[2]+upper[2]+lower[2]-ankle[2])
    # Vertical bone offset only, in-place. Root motion remains disabled in UE;
    # the capsule, canonical world movement and combat remain unchanged.
    return -max(0.,min(rises))
