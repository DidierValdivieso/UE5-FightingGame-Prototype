# M_RomboMasked - Cover SIN estirar (escalar U y V por separado)

El problema anterior: se escalaba U y V por el mismo valor, lo que solo hace zoom pero no corrige el aspect ratio.  
Para "cover" hay que escalar solo el eje que sobra: si la textura es más ancha (16:9), escalar solo U; si es más alta (9:16), escalar solo V.

---

## QUITAR la rama antigua (Multiply → Add que iba a ButtonTexture)

1. Elimina la conexión del **Multiply** al **Add**
2. Elimina la conexión del **Add** a **ButtonTexture**
3. Puedes dejar los nodos Max, Divide, etc. si quieres; ya no los usaremos para las UV

---

## NUEVA lógica: escalar U y V por separado

### Paso 1: CenteredUV (ya lo tienes)

- **TexCoord[0]** → **Subtract** ← **Constant2Vector (0.5, 0.5)**
- Salida del Subtract = CenteredUV

### Paso 2: Separar U y V

1. Clic derecho → **ComponentMask**
2. Conectar **CenteredUV** (salida del Subtract) a la entrada del ComponentMask
3. Marcar solo **R** y **G** (o **X** e **Y**)
4. O usa **Mask (R)** y **Mask (G)** por separado para obtener U y V

### Paso 3: scaleU = min(1, 1/TextureAspect)

1. **Divide**: A=1, B=TextureAspect → InvAspect (1/TextureAspect)
2. **Min**: A=1, B=InvAspect → scaleU
   - Si la textura es más ancha (TextureAspect > 1), scaleU = 1/TextureAspect
   - Si es más alta, scaleU = 1

### Paso 4: scaleV = min(1, TextureAspect)

1. **Min**: A=1, B=TextureAspect → scaleV
   - Si la textura es más alta (TextureAspect < 1), scaleV = TextureAspect
   - Si es más ancha, scaleV = 1

### Paso 5: Aplicar escalas a U y V

1. **Multiply**: A = U (del ComponentMask R), B = scaleU → U_scaled
2. **Multiply**: A = V (del ComponentMask G), B = scaleV → V_scaled

### Paso 6: Volver a centrar y recombinar

1. **Add**: A = U_scaled, B = 0.5 → U_final
2. **Add**: A = V_scaled, B = 0.5 → V_final
3. **AppendVector** (o **Make Float2**): A = U_final, B = V_final → UVs finales

### Paso 7: Conectar a ButtonTexture

- **UVs finales** (salida de AppendVector) → **UVs** de ButtonTexture

---

## Tabla de conexiones

| Nodo origen | Pin | Nodo destino | Pin |
|-------------|-----|--------------|-----|
| TexCoord[0] | out | Subtract | A |
| Const2Vec 0.5,0.5 | out | Subtract | B |
| Subtract | out | ComponentMask | In |
| ComponentMask | R | Multiply (U) | A |
| ComponentMask | G | Multiply (V) | A |
| Constant 1 | out | Divide | A |
| TextureAspect | out | Divide | B |
| Divide | out | Min (scaleU) | B |
| Constant 1 | out | Min (scaleU) | A |
| TextureAspect | out | Min (scaleV) | B |
| Constant 1 | out | Min (scaleV) | A |
| Min (scaleU) | out | Multiply (U) | B |
| Min (scaleV) | out | Multiply (V) | B |
| Multiply (U) | out | Add (U) | A |
| Multiply (V) | out | Add (V) | A |
| Constant 0.5 | out | Add (U) | B |
| Constant 0.5 | out | Add (V) | B |
| Add (U) | out | AppendVector | A (o X) |
| Add (V) | out | AppendVector | B (o Y) |
| AppendVector | out | ButtonTexture | UVs |

---

## Diagrama

```
                    TexCoord[0]
                         │
                         ▼
               Subtract ◄── (0.5, 0.5)
                    │
                    ▼
              ComponentMask (R, G)
                 │       │
                 │       │     TextureAspect
                 │       │          │
                 │       │    ┌─────┴─────┐
                 │       │    │           │
                 │       │    ▼           ▼
                 │       │  Divide     Min(1, TA) = scaleV
                 │       │  1÷TA            │
                 │       │    │             │
                 │       │    ▼             │
                 │       │  Min(1, 1÷TA) = scaleU
                 │       │    │             │
                 │       ▼    ▼             ▼
                 │  Multiply(U)        Multiply(V)
                 │  U × scaleU         V × scaleV
                 │    │                   │
                 │    ▼                   ▼
                 │  Add +0.5            Add +0.5
                 │    │                   │
                 │    └──────► AppendVector ◄─────┘
                 │                    │
                 └────────────────────┼─────────────► ButtonTexture UVs
```

---

## Nota sobre ComponentMask

Si usas **ComponentMask** con R y G activos, obtienes un Vector2. Para Multiply necesitas escalares:
- Usa **BreakFloat2Components** o **Mask (R)** y **Mask (G)** para obtener U y V por separado
- En Unreal: **Mask** con canal R da el valor U, **Mask** con canal G da el valor V

---

## Alternativa si no encuentras AppendVector

1. **Make Float2** o **Combine2**
2. O usa **TextureCoordinate** con **Custom** y mete U_final y V_final en los componentes
