# Starship

## controls

- up: activate main thrust engine
- left/right: activate left/right rotation engines
- down: automatic rotation stabilization (using rotation engines)
- ctrl: automatic rotation (using rotation engines) to invert current ship momentum
- shift: automatic rotation (using rotation engines) to acquire the closest enemy (display vector and distance)
- space: laser shot + launch auto-guided missiles (lock on the closest enemy)

## mechanisms

- any engines (thrust and rotation) consumes propellant, engines are stopped when empty
- propellant is slowly produced on idle
- celestial objects (ship, enemy, missiles) are subject to drag-forces dependings on design, taking in account:
	- shield capacity & quality
	- propellant capacity & quality
	- engines power & quality
- missiles explodes after some time without propellant
- laser/ships, laser/missiles (minus player's missiles), missiles/missiles, missiles/ships, ships/ships collisions & shield are properly handled
- shield is slowly repairing on idle
- enemies are regenerating randomly when destroyed

## notes

- currently, the player cannot die even if no more shield is active

## ToDo

- playability, use mouse?
- purpose?
- planets & gravity attraction
- mines (non-moving objects) with collisions
- information display (ship class, alerts...?)
- solar wind (waves)
- player death
