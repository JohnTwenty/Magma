# Magma: A Simple HLSL Playground
## Motivation
I am a guy who used to make a living with programming.  As many programmers I eventually got invited to do full time management at my company.  I started this project to prevent my coding skills from completely atrophying.  I was a graphics programming major back at school and had a few paid gigs doing graphics programming as a student.  That was mostly in the days before real time programmable shading.  Just after the first pixel and vertex shaders were invented I abandoned graphics in favor of other topics.  Fast-forward around twenty years, and GPUs have become nearly arbitrarily programmable.  

One day I thought it would be cool to dive back into doing some graphics coding on modern GPUs.  Most people by now were doing graphics programming -- especially for games using sophisticated engines like Unity or Unreal that took care of and hid a lot of the (for me, often interesting low level) complexity, and replaced that with their own (for me uninteresting higher level) complexity.  One project stood out for me as interesting and different: [Inigo Quilez](https://iquilezles.org/)'s [Shadertoy](https://www.shadertoy.com/) stood out as a web platform where I was able to write and share GPU shader code without any complexity whatsoever, just using a browser.  

I thought (and still think!) that writing shaders in Shadertoy was very, very cool, but I wanted to do a few things beyond what it let me do: 

* I was curious about the so called 'host side' graphics programming that shadertoy was completely hiding from me.  That is essentially the admittedly boring CPU side code that loads resources, compiles shaders, binds and populates buffers, and so on.  I wanted to be able to write that stuff myself too, primarily as a means about learning all aspects of modern graphics programming.

* Modern GPUs support at least three flavors of shaders: Vertex shaders, pixel shaders and compute shaders.  Shadertoy focuses exclusively on pixel shaders, but I wanted to be able to write and explore all three kinds.

* Finally, I am old school, and I prefer an offline out of browser experience.  Just my IDE, my compiler, my OS and hardware.  This is probably irrational, but I am old and set in my ways.

So with these goals established, I went to work.  I started this project late October 2013 as my personal Mercurial source control tells me.  And I worked on it very sporadically because I am very lazy.  Today in July 2023 as I write this, this Mercurial repo is on revision number 138.  That is something like, what, 13 changes per year, around one change per month?  Anyway, in the meantime the world nearly forgot about Mercurial and all the cool kids are putting their code on github.  I want to be a cool old man, so I thought it is time to put this old thing online.  

## What This Lets You Do


## Building