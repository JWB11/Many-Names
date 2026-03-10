import unreal

def log(msg):
    unreal.log('[Probe] ' + msg)

log('EditorActorSubsystem exists: ' + str(hasattr(unreal, 'EditorActorSubsystem')))
if hasattr(unreal, 'EditorActorSubsystem'):
    sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    log('EditorActorSubsystem methods: ' + ', '.join(sorted([n for n in dir(sub) if 'spawn' in n.lower()])))

world = unreal.EditorLevelLibrary.get_editor_world()
log('World class: ' + str(type(world)))
log('World methods with spawn: ' + ', '.join(sorted([n for n in dir(world) if 'spawn' in n.lower()])))
