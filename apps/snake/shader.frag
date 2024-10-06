#version 450 core

layout(location = 0) out vec4 outColor;

uniform uvec2 u_size;
uniform uint u_lines[32];

void main()
{
  int column = int(gl_FragCoord.x / float(u_size.x) * 32);
  int line = int(gl_FragCoord.y / float(u_size.y) * 32);

  if((u_lines[line] & (1 << column)) != 0)
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
  else
    outColor = vec4(0.0, 0.0, 0.0, 1.0);
}
